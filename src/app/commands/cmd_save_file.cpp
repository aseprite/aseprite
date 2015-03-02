// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_save_file.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/file/file.h"
#include "app/file_selector.h"
#include "app/job.h"
#include "app/modules/gui.h"
#include "app/recent_files.h"
#include "app/ui/status_bar.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/thread.h"
#include "base/unique_ptr.h"
#include "doc/sprite.h"
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
      fop_stop(m_fop);
    }

    waitJob();
  }

private:

  // Thread to do the hard work: save the file to the disk.
  virtual void onJob() override {
    try {
      fop_operate(m_fop, this);
    }
    catch (const std::exception& e) {
      fop_error(m_fop, "Error saving file:\n%s", e.what());
    }
    fop_done(m_fop);
  }

  virtual void ackFileOpProgress(double progress) override {
    jobProgress(progress);
  }

  FileOp* m_fop;
};

static void save_document_in_background(Context* context, Document* document,
  bool mark_as_saved, const std::string& fn_format)
{
  base::UniquePtr<FileOp> fop(fop_to_save_document(context, document,
      fn_format.c_str()));
  if (!fop)
    return;

  SaveFileJob job(fop);
  job.showProgressWindow();

  if (fop->has_error()) {
    Console console;
    console.printf(fop->error.c_str());

    // We don't know if the file was saved correctly or not. So mark
    // it as it should be saved again.
    document->impossibleToBackToSavedState();
  }
  // If the job was cancelled, mark the document as modified.
  else if (fop_is_stop(fop)) {
    document->impossibleToBackToSavedState();
  }
  else if (context->isUiAvailable()) {
    App::instance()->getRecentFiles()->addRecentFile(document->filename().c_str());
    if (mark_as_saved)
      document->markAsSaved();

    StatusBar::instance()
      ->setStatusText(2000, "File %s, saved.",
        document->name().c_str());
  }
}

//////////////////////////////////////////////////////////////////////

SaveFileBaseCommand::SaveFileBaseCommand(const char* short_name, const char* friendly_name, CommandFlags flags)
  : Command(short_name, friendly_name, flags)
{
}

void SaveFileBaseCommand::onLoadParams(Params* params)
{
  m_filename = params->get("filename");
  m_filenameFormat = params->get("filename-format");
}

// Returns true if there is a current sprite to save.
// [main thread]
bool SaveFileBaseCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void SaveFileBaseCommand::saveAsDialog(const ContextReader& reader, const char* dlgTitle, bool markAsSaved)
{
  const Document* document = reader.document();
  std::string filename;

  if (!m_filename.empty()) {
    filename = m_filename;
  }
  else {
    filename = document->filename();

    char exts[4096];
    get_writable_extensions(exts, sizeof(exts));

    for (;;) {
      std::string newfilename = app::show_file_selector(
        dlgTitle, filename, exts, FileSelectorType::Save);
      if (newfilename.empty())
        return;

      filename = newfilename;

      // Ask if the user wants overwrite the existent file.
      int ret = 0;
      if (base::is_file(filename)) {
        ret = ui::Alert::show("Warning<<The file already exists, overwrite it?<<%s||&Yes||&No||&Cancel",
          base::get_file_name(filename).c_str());

        // Check for read-only attribute.
        if (ret == 1) {
          if (!confirmReadonly(filename))
            ret = 2;              // Select file again.
          else
            break;
        }
      }
      else
        break;

      // "yes": we must continue with the operation...
      if (ret == 1) {
        break;
      }
      // "cancel" or <esc> per example: we back doing nothing
      else if (ret != 2)
        return;

      // "no": we must back to select other file-name
    }
  }

  {
    ContextWriter writer(reader);
    Document* documentWriter = writer.document();
    std::string oldFilename = documentWriter->filename();

    // Change the document file name
    documentWriter->setFilename(filename.c_str());
    m_selectedFilename = filename;

    // Save the document
    save_document_in_background(writer.context(), documentWriter,
      markAsSaved, m_filenameFormat);

    if (documentWriter->isModified())
      documentWriter->setFilename(oldFilename);

    update_screen_for_document(documentWriter);
  }
}

//static
bool SaveFileBaseCommand::confirmReadonly(const std::string& filename)
{
  if (!base::has_readonly_attr(filename))
    return true;

  int ret = ui::Alert::show("Warning<<The file is read-only, do you really want to overwrite it?<<%s||&Yes||&No",
    base::get_file_name(filename).c_str());

  if (ret == 1) {
    base::remove_readonly_attr(filename);
    return true;
  }
  else
    return false;
}

//////////////////////////////////////////////////////////////////////

class SaveFileCommand : public SaveFileBaseCommand {
public:
  SaveFileCommand();
  Command* clone() const override { return new SaveFileCommand(*this); }

protected:
  void onExecute(Context* context);
};

SaveFileCommand::SaveFileCommand()
  : SaveFileBaseCommand("SaveFile", "Save File", CmdRecordableFlag)
{
}

// Saves the active document in a file.
// [main thread]
void SaveFileCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Document* document(reader.document());

  // If the document is associated to a file in the file-system, we can
  // save it directly without user interaction.
  if (document->isAssociatedToFile()) {
    ContextWriter writer(reader);
    Document* documentWriter = writer.document();

    if (!confirmReadonly(documentWriter->filename()))
      return;

    save_document_in_background(context, documentWriter, true,
      m_filenameFormat.c_str());
    update_screen_for_document(documentWriter);
  }
  // If the document isn't associated to a file, we must to show the
  // save-as dialog to the user to select for first time the file-name
  // for this document.
  else {
    saveAsDialog(reader, "Save File", true);
  }
}

class SaveFileAsCommand : public SaveFileBaseCommand {
public:
  SaveFileAsCommand();
  Command* clone() const override { return new SaveFileAsCommand(*this); }

protected:
  void onExecute(Context* context);
};

SaveFileAsCommand::SaveFileAsCommand()
  : SaveFileBaseCommand("SaveFileAs", "Save File As", CmdRecordableFlag)
{
}

void SaveFileAsCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  saveAsDialog(reader, "Save As", true);
}

class SaveFileCopyAsCommand : public SaveFileBaseCommand {
public:
  SaveFileCopyAsCommand();
  Command* clone() const override { return new SaveFileCopyAsCommand(*this); }

protected:
  void onExecute(Context* context);
};

SaveFileCopyAsCommand::SaveFileCopyAsCommand()
  : SaveFileBaseCommand("SaveFileCopyAs", "Save File Copy As", CmdRecordableFlag)
{
}

void SaveFileCopyAsCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Document* document(reader.document());
  std::string old_filename = document->filename();

  // show "Save As" dialog
  saveAsDialog(reader, "Save Copy As", false);

  // Restore the file name.
  {
    ContextWriter writer(reader);
    writer.document()->setFilename(old_filename.c_str());
    update_screen_for_document(writer.document());
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
