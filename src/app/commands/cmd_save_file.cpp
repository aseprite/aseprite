// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_save_file.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/file/file.h"
#include "app/file_selector.h"
#include "app/job.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/restore_visible_layers.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "app/ui/status_bar.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/thread.h"
#include "base/unique_ptr.h"
#include "doc/frame_tag.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

class SaveAsCopyDelegate : public FileSelectorDelegate {
public:
  SaveAsCopyDelegate(const Sprite* sprite,
                     const double scale,
                     const std::string& layer,
                     const std::string& frame,
                     const bool applyPixelRatio)
    : m_sprite(sprite),
      m_resizeScale(scale),
      m_layer(layer),
      m_frame(frame),
      m_applyPixelRatio(applyPixelRatio) { }

  bool hasResizeCombobox() override {
    return true;
  }

  double getResizeScale() override {
    return m_resizeScale;
  }

  void setResizeScale(double scale) override {
    m_resizeScale = scale;
  }

  void fillLayersComboBox(ui::ComboBox* layers) override {
    fill_layers_combobox(m_sprite, layers, m_layer);
  }

  void fillFramesComboBox(ui::ComboBox* frames) override {
    fill_frames_combobox(m_sprite, frames, m_frame);
  }

  std::string getLayers() override { return m_layer; }
  std::string getFrames() override { return m_frame; }

  void setLayers(const std::string& layer) override {
    m_layer = layer;
  }

  void setFrames(const std::string& frame) override {
    m_frame = frame;
  }

  void setApplyPixelRatio(bool applyPixelRatio) override {
    m_applyPixelRatio = applyPixelRatio;
  }

  bool applyPixelRatio() const override {
    return m_applyPixelRatio;
  }

  doc::PixelRatio pixelRatio() override {
    return m_sprite->pixelRatio();
  }

private:
  const Sprite* m_sprite;
  double m_resizeScale;
  std::string m_layer;
  std::string m_frame;
  bool m_applyPixelRatio;
};

class SaveFileJob : public Job, public IFileOpProgress {
public:
  SaveFileJob(FileOp* fop)
    : Job("Saving file")
    , m_fop(fop)
  {
  }

  void showProgressWindow() {
    startJob();

    if (isCanceled()) {
      m_fop->stop();
    }

    waitJob();
  }

private:

  // Thread to do the hard work: save the file to the disk.
  virtual void onJob() override {
    try {
      m_fop->operate(this);
    }
    catch (const std::exception& e) {
      m_fop->setError("Error saving file:\n%s", e.what());
    }
    m_fop->done();
  }

  virtual void ackFileOpProgress(double progress) override {
    jobProgress(progress);
  }

  FileOp* m_fop;
};

//////////////////////////////////////////////////////////////////////

SaveFileBaseCommand::SaveFileBaseCommand(const char* short_name, const char* friendly_name, CommandFlags flags)
  : Command(short_name, friendly_name, flags)
{
}

void SaveFileBaseCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");
  m_filenameFormat = params.get("filename-format");
  m_frameTag = params.get("frame-tag");
  m_slice = params.get("slice");

  if (params.has_param("from-frame") ||
      params.has_param("to-frame")) {
    doc::frame_t fromFrame = params.get_as<doc::frame_t>("from-frame");
    doc::frame_t toFrame = params.get_as<doc::frame_t>("to-frame");
    m_selFrames.insert(fromFrame, toFrame);
    m_adjustFramesByFrameTag = true;
  }
  else {
    m_selFrames.clear();
    m_adjustFramesByFrameTag = false;
  }
}

// Returns true if there is a current sprite to save.
// [main thread]
bool SaveFileBaseCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

bool SaveFileBaseCommand::saveAsDialog(
  Context* context,
  const std::string& dlgTitle,
  const std::string& forbiddenFilename,
  FileSelectorDelegate* delegate)
{
  const Document* document = context->activeDocument();
  std::string filename;

  // If there is a delegate, we're doing a "Save Copy As/Export", so we don't
  // have to mark the file as saved.
  const bool isExport = (delegate != nullptr);
  const bool markAsSaved = (!isExport);
  double xscale = 1.0;
  double yscale = 1.0;

  if (!m_filename.empty()) {
    filename = m_filename;
  }
  else {
    std::string exts = get_writable_extensions();
    filename = document->filename();

  again:;
    FileSelectorFiles newfilename;
    if (!app::show_file_selector(
          dlgTitle, filename, exts,
          FileSelectorType::Save, newfilename,
          delegate))
      return false;

    filename = newfilename.front();
    if (base::normalize_path(forbiddenFilename) ==
        base::normalize_path(filename)) {
      ui::Alert::show("Overwrite Warning"
                      "<<You cannot save a copy with the same name (overwrite the original file)."
                      "<<Use File > Save menu option in that case."
                      "||&OK");
      goto again;
    }

    if (delegate &&
        delegate->hasResizeCombobox()) {
      xscale = yscale = delegate->getResizeScale();
    }
  }

  std::string oldFilename;
  {
    ContextWriter writer(context);
    Document* documentWriter = writer.document();
    oldFilename = documentWriter->filename();

    // Change the document file name
    documentWriter->setFilename(filename.c_str());
    m_selectedFilename = filename;
  }

  // Pixel ratio
  if (delegate &&
      delegate->applyPixelRatio()) {
    doc::PixelRatio pr = delegate->pixelRatio();
    xscale *= pr.w;
    yscale *= pr.h;
  }

  // Apply scale
  bool undoResize = false;
  if (xscale != 1.0 || yscale != 1.0) {
    Command* resizeCmd = CommandsModule::instance()->getCommandByName(CommandId::SpriteSize);
    ASSERT(resizeCmd);
    if (resizeCmd) {
      int width = document->sprite()->width();
      int height = document->sprite()->height();
      int newWidth = int(double(width) * xscale);
      int newHeight = int(double(height) * yscale);
      if (newWidth < 1) newWidth = 1;
      if (newHeight < 1) newHeight = 1;
      if (width != newWidth || height != newHeight) {
        Params params;
        params.set("use-ui", "false");
        params.set("width", base::convert_to<std::string>(newWidth).c_str());
        params.set("height", base::convert_to<std::string>(newHeight).c_str());
        params.set("resize-method", "nearest-neighbor"); // TODO add algorithm in the UI?
        context->executeCommand(resizeCmd, params);
        undoResize = true;
      }
    }
  }

  {
    RestoreVisibleLayers layersVisibility;
    if (delegate) {
      Site site = context->activeSite();

      // Selected layers to export
      calculate_visible_layers(site,
                               delegate->getLayers(),
                               layersVisibility);

      // Selected frames to export
      SelectedFrames selFrames;
      FrameTag* frameTag = calculate_selected_frames(
        site, delegate->getFrames(), selFrames);
      if (frameTag)
        m_frameTag = frameTag->name();
      m_selFrames = selFrames;
      m_adjustFramesByFrameTag = false;
    }

    // Save the document
    saveDocumentInBackground(context, const_cast<Document*>(document), markAsSaved);
  }

  // Undo resize
  if (undoResize) {
    Command* undoCmd = CommandsModule::instance()->getCommandByName(CommandId::Undo);
    if (undoCmd)
      context->executeCommand(undoCmd);
  }

  {
    ContextWriter writer(context);
    Document* documentWriter = writer.document();

    if (document->isModified())
      documentWriter->setFilename(oldFilename);
    else
      documentWriter->incrementVersion();
  }

  return true;
}

void SaveFileBaseCommand::saveDocumentInBackground(const Context* context,
                                                   const app::Document* document,
                                                   bool markAsSaved) const
{
  base::UniquePtr<FileOp> fop(
    FileOp::createSaveDocumentOperation(
      context,
      FileOpROI(document, m_slice, m_frameTag,
                m_selFrames, m_adjustFramesByFrameTag),
      document->filename(),
      m_filenameFormat));
  if (!fop)
    return;

  SaveFileJob job(fop);
  job.showProgressWindow();

  if (fop->hasError()) {
    Console console;
    console.printf(fop->error().c_str());

    // We don't know if the file was saved correctly or not. So mark
    // it as it should be saved again.
    const_cast<Document*>(document)->impossibleToBackToSavedState();
  }
  // If the job was cancelled, mark the document as modified.
  else if (fop->isStop()) {
    const_cast<Document*>(document)->impossibleToBackToSavedState();
  }
  else if (context->isUIAvailable()) {
    App::instance()->recentFiles()->addRecentFile(document->filename().c_str());
    if (markAsSaved)
      const_cast<Document*>(document)->markAsSaved();

    StatusBar::instance()
      ->setStatusText(2000, "File %s, saved.",
        document->name().c_str());
  }
}

//////////////////////////////////////////////////////////////////////

class SaveFileCommand : public SaveFileBaseCommand {
public:
  SaveFileCommand();
  Command* clone() const override { return new SaveFileCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

SaveFileCommand::SaveFileCommand()
  : SaveFileBaseCommand("SaveFile", "Save File", CmdRecordableFlag)
{
}

// Saves the active document in a file.
// [main thread]
void SaveFileCommand::onExecute(Context* context)
{
  Document* document = context->activeDocument();

  // If the document is associated to a file in the file-system, we can
  // save it directly without user interaction.
  if (document->isAssociatedToFile()) {
    ContextWriter writer(context);
    Document* documentWriter = writer.document();

    saveDocumentInBackground(context, documentWriter, true);
  }
  // If the document isn't associated to a file, we must to show the
  // save-as dialog to the user to select for first time the file-name
  // for this document.
  else {
    saveAsDialog(context, "Save File");
  }
}

class SaveFileAsCommand : public SaveFileBaseCommand {
public:
  SaveFileAsCommand();
  Command* clone() const override { return new SaveFileAsCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

SaveFileAsCommand::SaveFileAsCommand()
  : SaveFileBaseCommand("SaveFileAs", "Save File As", CmdRecordableFlag)
{
}

void SaveFileAsCommand::onExecute(Context* context)
{
  saveAsDialog(context, "Save As");
}

class SaveFileCopyAsCommand : public SaveFileBaseCommand {
public:
  SaveFileCopyAsCommand();
  Command* clone() const override { return new SaveFileCopyAsCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

SaveFileCopyAsCommand::SaveFileCopyAsCommand()
  : SaveFileBaseCommand("SaveFileCopyAs", "Export", CmdRecordableFlag)
{
}

void SaveFileCopyAsCommand::onExecute(Context* context)
{
  const Document* document = context->activeDocument();
  std::string oldFilename = document->filename();

  // show "Save As" dialog
  DocumentPreferences& docPref = Preferences::instance().document(document);

  base::UniquePtr<SaveAsCopyDelegate> delegate;
  if (context->isUIAvailable()) {
    delegate.reset(
      new SaveAsCopyDelegate(
        document->sprite(),
        docPref.saveCopy.resizeScale(),
        docPref.saveCopy.layer(),
        docPref.saveCopy.frameTag(),
        docPref.saveCopy.applyPixelRatio()));
  }

  // Is a default output filename in the preferences?
  if (!docPref.saveCopy.filename().empty()) {
    ContextWriter writer(context);
    writer.document()->setFilename(
      docPref.saveCopy.filename());
  }

  if (saveAsDialog(context, "Export",
                   oldFilename, delegate)) {
    docPref.saveCopy.filename(document->filename());
    if (delegate) {
      docPref.saveCopy.resizeScale(delegate->getResizeScale());
      docPref.saveCopy.layer(delegate->getLayers());
      docPref.saveCopy.frameTag(delegate->getFrames());
      docPref.saveCopy.applyPixelRatio(delegate->applyPixelRatio());
    }
  }

  // Restore the file name.
  {
    ContextWriter writer(context);
    writer.document()->setFilename(oldFilename.c_str());
  }
}

Command* CommandFactory::createSaveFileCommand()
{
  return new SaveFileCommand;
}

Command* CommandFactory::createSaveFileAsCommand()
{
  return new SaveFileAsCommand;
}

Command* CommandFactory::createSaveFileCopyAsCommand()
{
  return new SaveFileCopyAsCommand;
}

} // namespace app
