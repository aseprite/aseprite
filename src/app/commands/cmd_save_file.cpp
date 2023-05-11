// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
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
#include "app/doc.h"
#include "app/doc_undo.h"
#include "app/file/file.h"
#include "app/file/gif_format.h"
#include "app/file/png_format.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/job.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/restore_visible_layers.h"
#include "app/ui/export_file_window.h"
#include "app/ui/incompat_file_window.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "app/ui/optional_alert.h"
#include "app/ui/status_bar.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/scoped_value.h"
#include "base/thread.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "fmt/format.h"
#include "ui/ui.h"
#include "undo/undo_state.h"

namespace app {

class SaveFileJob : public Job, public IFileOpProgress {
public:
  SaveFileJob(FileOp* fop)
    : Job(Strings::save_file_saving().c_str())
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
  : CommandWithNewParams<SaveFileParams>(id, flags)
{
}

void SaveFileBaseCommand::onLoadParams(const Params& params)
{
  CommandWithNewParams<SaveFileParams>::onLoadParams(params);

  if (this->params().fromFrame.isSet() ||
      this->params().toFrame.isSet()) {
    doc::frame_t fromFrame = this->params().fromFrame();
    doc::frame_t toFrame = this->params().toFrame();
    m_selFrames.insert(fromFrame, toFrame);
    m_adjustFramesByTag = true;
  }
  else {
    m_selFrames.clear();
    m_adjustFramesByTag = false;
  }
}

// Returns true if there is a current sprite to save.
// [main thread]
bool SaveFileBaseCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

std::string SaveFileBaseCommand::saveAsDialog(
  Context* context,
  const std::string& dlgTitle,
  const std::string& initialFilename,
  const MarkAsSaved markAsSaved,
  const SaveInBackground saveInBackground,
  const std::string& forbiddenFilename)
{
  Doc* document = context->activeDocument();

  // Before we change the document filename to the copy, we save its
  // preferences so in a future export operation the values persist,
  // and we can re-export the original document with the same
  // preferences.
  Preferences::instance().save();

  std::string filename = params().filename();
  if (filename.empty() || params().ui()) {
    base::paths exts = get_writable_extensions();
    filename = initialFilename;

#ifdef ENABLE_UI
    if (context->isUIAvailable()) {
    again:;
      base::paths newfilename;
      if (!params().ui() ||
          !app::show_file_selector(
            dlgTitle, filename, exts,
            FileSelectorType::Save,
            newfilename)) {
        return std::string();
      }

      filename = newfilename.front();
      if (!forbiddenFilename.empty() &&
          base::normalize_path(forbiddenFilename) ==
          base::normalize_path(filename)) {
        ui::Alert::show(Strings::alerts_cannot_file_overwrite_on_export());
        goto again;
      }
    }
#endif // ENABLE_UI
  }

  if (filename.empty())
    return std::string();

  // Document is being saved under a different path/name, remove
  // read-only mark then.
  if (filename != initialFilename) {
    document->removeReadOnlyMark();
  }

  if (saveInBackground == SaveInBackground::On) {
    saveDocumentInBackground(
      context, document,
      filename, markAsSaved);

    // Reset the "saveCopy" document preferences of the new document
    // (here "document" contains the new filename), because these
    // preferences make sense only for the original document that was
    // exported/copied, not for the new one.
    //
    // The new document (the copy) must have the default preferences
    // just in case the user want to export it to other file (so a
    // proper default export filename is calculated). This scenario is
    // described here:
    //
    //   https://github.com/aseprite/aseprite/issues/1964
    //
    auto& docPref = Preferences::instance().document(document);
    docPref.saveCopy.clearSection();
  }

  return filename;
}

void SaveFileBaseCommand::saveDocumentInBackground(
  const Context* context,
  Doc* document,
  const std::string& filename,
  const MarkAsSaved markAsSaved,
  const ResizeOnTheFly resizeOnTheFly,
  const gfx::PointF& scale)
{
#ifdef ENABLE_UI
  // If the document is read only, we cannot save it directly (we have
  // to use File > Save As)
  if (document->isReadOnly() &&
      context->isUIAvailable()) {
    IncompatFileWindow window;
    window.show();
    return;
  }
#endif // ENABLE_UI

  if (params().aniDir.isSet()) {
    switch (params().aniDir()) {
      case AniDir::REVERSE:
        m_selFrames = m_selFrames.makeReverse();
        break;
      case AniDir::PING_PONG:
        m_selFrames = m_selFrames.makePingPong();
        break;
      case AniDir::PING_PONG_REVERSE:
        m_selFrames = m_selFrames.makePingPong();
        m_selFrames = m_selFrames.makeReverse();
        break;
    }
  }

  gfx::Rect bounds;
  if (params().bounds.isSet())
    bounds = params().bounds();

  FileOpROI roi(document, bounds,
                params().slice(), params().tag(),
                m_selFrames, m_adjustFramesByTag);

  std::unique_ptr<FileOp> fop(
    FileOp::createSaveDocumentOperation(
      context,
      roi,
      filename,
      params().filenameFormat(),
      params().ignoreEmpty()));
  if (!fop)
    return;

  if (resizeOnTheFly == ResizeOnTheFly::On)
    fop->setOnTheFlyScale(scale);

  SaveFileJob job(fop.get());
  job.showProgressWindow();

  if (fop->hasError()) {
    Console console;
    console.printf(fop->error().c_str());

    // We don't know if the file was saved correctly or not. So mark
    // it as it should be saved again. Except for read-only documents,
    // since we know they must not be saved.
    if (!document->isReadOnly())
      document->impossibleToBackToSavedState();
  }
  // If the job was cancelled, mark the document as modified.
  else if (fop->isStop()) {
    document->impossibleToBackToSavedState();
  }
  else {
    if (context->isUIAvailable() && params().ui())
      App::instance()->recentFiles()->addRecentFile(filename);

    if (markAsSaved == MarkAsSaved::On) {
      document->markAsSaved();
      document->setFilename(filename);
      document->incrementVersion();
    }

#ifdef ENABLE_UI
    if (context->isUIAvailable() && params().ui()) {
      StatusBar::instance()->setStatusText(
        2000, fmt::format(Strings::save_file_saved(),
                          base::get_file_name(filename)));
    }
#endif
  }
}

//////////////////////////////////////////////////////////////////////

class SaveFileCommand : public SaveFileBaseCommand {
public:
  SaveFileCommand();

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
  Doc* document = context->activeDocument();

  // If the document is associated to a file in the file-system, we can
  // save it directly without user interaction.
  if (document->isAssociatedToFile()) {
    const ContextReader reader(context);
    const Doc* documentReader = reader.document();

    saveDocumentInBackground(
      context, document,
      (params().filename.isSet() ? params().filename():
                                   documentReader->filename()),
      MarkAsSaved::On);
  }
  // If the document isn't associated to a file, we must to show the
  // save-as dialog to the user to select for first time the file-name
  // for this document.
  else {
    saveAsDialog(context, Strings::save_file_title(),
                 (params().filename.isSet() ? params().filename():
                                              document->filename()),
                 MarkAsSaved::On);
  }
}

class SaveFileAsCommand : public SaveFileBaseCommand {
public:
  SaveFileAsCommand();

protected:
  void onExecute(Context* context) override;
};

SaveFileAsCommand::SaveFileAsCommand()
  : SaveFileBaseCommand(CommandId::SaveFileAs(), CmdRecordableFlag)
{
}

void SaveFileAsCommand::onExecute(Context* context)
{
  Doc* document = context->activeDocument();
  saveAsDialog(context, Strings::save_file_save_as(),
               (params().filename.isSet() ? params().filename():
                                            document->filename()),
               MarkAsSaved::On);
}

class SaveFileCopyAsCommand : public SaveFileBaseCommand {
public:
  SaveFileCopyAsCommand();

protected:
  void onExecute(Context* context) override;

private:
  void moveToUndoState(Doc* doc,
                       const undo::UndoState* state);
};

SaveFileCopyAsCommand::SaveFileCopyAsCommand()
  : SaveFileBaseCommand(CommandId::SaveFileCopyAs(), CmdRecordableFlag)
{
}

void SaveFileCopyAsCommand::onExecute(Context* context)
{
  Doc* doc = context->activeDocument();
  std::string outputFilename = params().filename();
  std::string layers = kAllLayers;
  int layersIndex = -1;
  std::string frames = kAllFrames;
  bool applyPixelRatio = false;
  double scale = params().scale();
  gfx::Rect bounds = params().bounds();
  doc::AniDir aniDirValue = params().aniDir();
  bool isForTwitter = false;

#if ENABLE_UI
  if (params().ui() && context->isUIAvailable()) {
    ExportFileWindow win(doc);
    bool askOverwrite = true;

    win.SelectOutputFile.connect(
      [this, &win, &askOverwrite, context, doc]() -> std::string {
        std::string result =
          saveAsDialog(
            context, Strings::save_file_export(),
            win.outputFilenameValue(),
            MarkAsSaved::Off,
            SaveInBackground::Off,
            (doc->isAssociatedToFile() ? doc->filename():
                                         std::string()));
        if (!result.empty())
          askOverwrite = false; // Already asked in the file selector dialog

        return result;
      });

    if (params().filename.isSet()) {
      std::string outputPath = base::get_file_path(outputFilename);
      if (outputPath.empty()) {
        outputPath = base::get_file_path(doc->filename());
        outputFilename = base::join_path(outputPath, outputFilename);
      }
      win.setOutputFilename(outputFilename);
    }

    if (params().scale.isSet()) win.setResizeScale(scale);
    if (params().aniDir.isSet()) win.setAniDir(aniDirValue);

    if (params().slice.isSet()) win.setArea(params().slice());
    else if (params().bounds.isSet() &&
             doc->isMaskVisible() &&
             doc->mask()->bounds() == params().bounds()) {
      win.setArea(kSelectedCanvas);
    }

    win.remapWindow();
    load_window_pos(&win, "ExportFile");
  again:;
    const bool result = win.show();
    save_window_pos(&win, "ExportFile");
    if (!result)
      return;

    outputFilename = win.outputFilenameValue();

    if (askOverwrite &&
        base::is_file(outputFilename)) {
      int ret = OptionalAlert::show(
        Preferences::instance().exportFile.showOverwriteFilesAlert,
        1, // Yes is the default option when the alert dialog is disabled
        fmt::format(Strings::alerts_overwrite_files_on_export(),
                    outputFilename));
      if (ret != 1)
        goto again;
    }

    // Save the preferences used to export the file, so if we open the
    // window again, we will have the same options.
    win.savePref();

    layers = win.layersValue();
    layersIndex = win.layersIndex();
    frames = win.framesValue();
    scale = win.resizeValue();
    params().slice(win.areaValue()); // Set slice
    if (win.areaValue() == kSelectedCanvas && doc->isMaskVisible())
      bounds = doc->mask()->bounds();
    applyPixelRatio = win.applyPixelRatio();
    aniDirValue = win.aniDirValue();
    isForTwitter = win.isForTwitter();
  }
#endif

  gfx::PointF scaleXY(scale, scale);

  // Pixel ratio
  if (applyPixelRatio) {
    doc::PixelRatio pr = doc->sprite()->pixelRatio();
    scaleXY.x *= pr.w;
    scaleXY.y *= pr.h;
  }

  // First of all we'll try to use the "on the fly" scaling, to avoid
  // using a resize command to apply the export scale.
  const undo::UndoState* undoState = nullptr;
  bool undoResize = false;
  const bool resizeOnTheFly = FileOp::checkIfFormatSupportResizeOnTheFly(outputFilename);
  if (!resizeOnTheFly && (scaleXY.x != 1.0 ||
                          scaleXY.y != 1.0)) {
    Command* resizeCmd = Commands::instance()->byId(CommandId::SpriteSize());
    ASSERT(resizeCmd);
    if (resizeCmd) {
      int width = doc->sprite()->width();
      int height = doc->sprite()->height();
      int newWidth = int(double(width) * scaleXY.x);
      int newHeight = int(double(height) * scaleXY.y);
      if (newWidth < 1) newWidth = 1;
      if (newHeight < 1) newHeight = 1;
      if (width != newWidth || height != newHeight) {
        doc->setInhibitBackup(true);
        undoState = doc->undoHistory()->currentState();
        undoResize = true;

        Params params;
        params.set("use-ui", "false");
        params.set("width", base::convert_to<std::string>(newWidth).c_str());
        params.set("height", base::convert_to<std::string>(newHeight).c_str());
        params.set("resize-method", "nearest-neighbor"); // TODO add algorithm in the UI?
        context->executeCommand(resizeCmd, params);
      }
    }
  }

  {
    RestoreVisibleLayers layersVisibility;
    if (context->isUIAvailable()) {
      Site site = context->activeSite();

      // Selected layers to export
      calculate_visible_layers(site,
                               layers,
                               layersIndex,
                               layersVisibility);

      // m_selFrames is not empty if fromFrame/toFrame parameters are
      // specified.
      if (m_selFrames.empty()) {
        // Selected frames to export
        SelectedFrames selFrames;
        Tag* tag = calculate_selected_frames(
          site, frames, selFrames);
        if (tag)
          params().tag(tag->name());
        m_selFrames = selFrames;
      }
      m_adjustFramesByTag = false;
    }

    // Set other parameters
    params().aniDir(aniDirValue);
    if (!bounds.isEmpty())
      params().bounds(bounds);

    // TODO This should be set as options for the specific encoder
    GifEncoderDurationFix fixGif(isForTwitter);
    PngEncoderOneAlphaPixel fixPng(isForTwitter);

    saveDocumentInBackground(
      context, doc, outputFilename,
      MarkAsSaved::Off,
      (resizeOnTheFly ? ResizeOnTheFly::On:
                        ResizeOnTheFly::Off),
      scaleXY);
  }

  // Undo resize
  if (undoResize &&
      undoState != doc->undoHistory()->currentState()) {
    moveToUndoState(doc, undoState);
    doc->setInhibitBackup(false);
  }
}

void SaveFileCopyAsCommand::moveToUndoState(Doc* doc,
                                            const undo::UndoState* state)
{
  try {
    DocWriter writer(doc, 100);
    doc->undoHistory()->moveToState(state);
    doc->generateMaskBoundaries();
    doc->notifyGeneralUpdate();
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
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
