// Aseprite
// Copyright (C) 2001-2018  David Capello
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
#include "app/i18n/strings.h"
#include "app/job.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/restore_visible_layers.h"
#include "app/ui/export_file_window.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "app/ui/optional_alert.h"
#include "app/ui/status_bar.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/thread.h"
#include "base/unique_ptr.h"
#include "doc/frame_tag.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "ui/ui.h"

namespace app {

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

SaveFileBaseCommand::SaveFileBaseCommand(const char* id, CommandFlags flags)
  : Command(id, flags)
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

std::string SaveFileBaseCommand::saveAsDialog(
  Context* context,
  const std::string& dlgTitle,
  const std::string& initialFilename,
  const bool markAsSaved,
  const bool saveInBackground,
  const std::string& forbiddenFilename)
{
  Document* document = context->activeDocument();
  std::string filename;

  if (!m_filename.empty()) {
    filename = m_filename;
  }
  else {
    base::paths exts = get_writable_extensions();
    filename = initialFilename;

  again:;
    base::paths newfilename;
    if (!app::show_file_selector(
          dlgTitle, filename, exts,
          FileSelectorType::Save,
          newfilename))
      return std::string();

    filename = newfilename.front();
    if (!forbiddenFilename.empty() &&
        base::normalize_path(forbiddenFilename) ==
        base::normalize_path(filename)) {
      ui::Alert::show(Strings::alerts_cannot_file_overwrite_on_export());
      goto again;
    }
  }

  if (saveInBackground) {
    saveDocumentInBackground(
      context, document,
      filename, markAsSaved);
  }

  return filename;
}

void SaveFileBaseCommand::saveDocumentInBackground(
  const Context* context,
  app::Document* document,
  const std::string& filename,
  const bool markAsSaved) const
{
  base::UniquePtr<FileOp> fop(
    FileOp::createSaveDocumentOperation(
      context,
      FileOpROI(document, m_slice, m_frameTag,
                m_selFrames, m_adjustFramesByFrameTag),
      filename,
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
    document->impossibleToBackToSavedState();
  }
  // If the job was cancelled, mark the document as modified.
  else if (fop->isStop()) {
    document->impossibleToBackToSavedState();
  }
  else if (context->isUIAvailable()) {
    App::instance()->recentFiles()->addRecentFile(filename);
    if (markAsSaved) {
      document->markAsSaved();
      document->setFilename(filename);
      document->incrementVersion();
    }
    StatusBar::instance()
      ->setStatusText(2000, "File <%s> saved.",
        base::get_file_name(filename).c_str());
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
  : SaveFileBaseCommand(CommandId::SaveFile(), CmdRecordableFlag)
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
    const ContextReader reader(context);
    const Document* documentReader = reader.document();

    saveDocumentInBackground(
      context, document,
      documentReader->filename(), true);
  }
  // If the document isn't associated to a file, we must to show the
  // save-as dialog to the user to select for first time the file-name
  // for this document.
  else {
    saveAsDialog(context, "Save File",
                 document->filename(), true);
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
  : SaveFileBaseCommand(CommandId::SaveFileAs(), CmdRecordableFlag)
{
}

void SaveFileAsCommand::onExecute(Context* context)
{
  Document* document = context->activeDocument();
  saveAsDialog(context, "Save As",
               document->filename(), true);
}

class SaveFileCopyAsCommand : public SaveFileBaseCommand {
public:
  SaveFileCopyAsCommand();
  Command* clone() const override { return new SaveFileCopyAsCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

SaveFileCopyAsCommand::SaveFileCopyAsCommand()
  : SaveFileBaseCommand(CommandId::SaveFileCopyAs(), CmdRecordableFlag)
{
}

void SaveFileCopyAsCommand::onExecute(Context* context)
{
  Document* doc = context->activeDocument();
  ExportFileWindow win(doc);
  bool askOverwrite = true;

  win.SelectOutputFile.connect(
    [this, &win, &askOverwrite, context, doc]() -> std::string {
      std::string result =
        saveAsDialog(
          context, "Export",
          win.outputFilenameValue(), false, false,
          (doc->isAssociatedToFile() ? doc->filename():
                                       std::string()));
      if (!result.empty())
        askOverwrite = false; // Already asked in the file selector dialog

      return result;
    });

again:;
  if (!win.show())
    return;

  if (askOverwrite) {
    int ret = OptionalAlert::show(
      Preferences::instance().exportFile.showOverwriteFilesAlert,
      1, // Yes is the default option when the alert dialog is disabled
      fmt::format(Strings::alerts_overwrite_files_on_export(),
                  win.outputFilenameValue()));
    if (ret != 1)
      goto again;
  }

  // Save the preferences used to export the file, so if we open the
  // window again, we will have the same options.
  win.savePref();

  double xscale, yscale;
  xscale = yscale = win.resizeValue();

  // Pixel ratio
  if (win.applyPixelRatio()) {
    doc::PixelRatio pr = doc->sprite()->pixelRatio();
    xscale *= pr.w;
    yscale *= pr.h;
  }

  // Apply scale
  bool undoResize = false;
  if (xscale != 1.0 || yscale != 1.0) {
    Command* resizeCmd = Commands::instance()->byId(CommandId::SpriteSize());
    ASSERT(resizeCmd);
    if (resizeCmd) {
      int width = doc->sprite()->width();
      int height = doc->sprite()->height();
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
    Site site = context->activeSite();

    // Selected layers to export
    calculate_visible_layers(site,
                             win.layersValue(),
                             layersVisibility);

    // Selected frames to export
    SelectedFrames selFrames;
    FrameTag* frameTag = calculate_selected_frames(
      site, win.framesValue(), selFrames);
    if (frameTag)
      m_frameTag = frameTag->name();
    m_selFrames = selFrames;
    m_adjustFramesByFrameTag = false;

    saveDocumentInBackground(
      context, doc, win.outputFilenameValue(), false);
  }

  // Undo resize
  if (undoResize) {
    Command* undoCmd = Commands::instance()->byId(CommandId::Undo());
    if (undoCmd)
      context->executeCommand(undoCmd);
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
