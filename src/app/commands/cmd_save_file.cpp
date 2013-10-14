/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
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
#include "raster/sprite.h"
#include "ui/ui.h"

static const int kMonitoringPeriod = 100;

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
    fop_stop(m_fop);
  }

private:

  // Thread to do the hard work: save the file to the disk.
  virtual void onJob() OVERRIDE {
    try {
      fop_operate(m_fop, this);
    }
    catch (const std::exception& e) {
      fop_error(m_fop, "Error saving file:\n%s", e.what());
    }
    fop_done(m_fop);
  }

  virtual void ackFileOpProgress(double progress) OVERRIDE {
    jobProgress(progress);
  }

  FileOp* m_fop;
};

static void save_document_in_background(Document* document, bool mark_as_saved)
{
  base::UniquePtr<FileOp> fop(fop_to_save_document(document));
  if (!fop)
    return;

  SaveFileJob job(fop);
  job.showProgressWindow();

  if (fop->has_error()) {
    Console console;
    console.printf(fop->error.c_str());
  }
  else {
    App::instance()->getRecentFiles()->addRecentFile(document->getFilename().c_str());
    if (mark_as_saved)
      document->markAsSaved();

    StatusBar::instance()
      ->setStatusText(2000, "File %s, saved.",
                      base::get_file_name(document->getFilename()));
  }
}

//////////////////////////////////////////////////////////////////////

static void save_as_dialog(const ContextReader& reader, const char* dlg_title, bool mark_as_saved)
{
  const Document* document = reader.document();
  char exts[4096];
  base::string filename;
  base::string newfilename;
  int ret;

  filename = document->getFilename();
  get_writable_extensions(exts, sizeof(exts));

  for (;;) {
    newfilename = app::show_file_selector(dlg_title, filename, exts);
    if (newfilename.empty())
      return;

    filename = newfilename;

    if (base::file_exists(filename.c_str())) {
      // Ask if the user wants overwrite the existent file?
      ret = ui::Alert::show("Warning<<File exists, overwrite it?<<%s||&Yes||&No||&Cancel",
                            base::get_file_name(filename).c_str());
    }
    else
      break;

    // "yes": we must continue with the operation...
    if (ret == 1)
      break;
    // "cancel" or <esc> per example: we back doing nothing
    else if (ret != 2)
      return;

    // "no": we must back to select other file-name
  }

  {
    ContextWriter writer(reader);
    Document* documentWriter = writer.document();

    // Change the document file name
    documentWriter->setFilename(filename.c_str());

    // Save the document
    save_document_in_background(documentWriter, mark_as_saved);

    update_screen_for_document(documentWriter);
  }
}

class SaveFileCommand : public Command {
public:
  SaveFileCommand();
  Command* clone() { return new SaveFileCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

SaveFileCommand::SaveFileCommand()
  : Command("SaveFile",
            "Save File",
            CmdRecordableFlag)
{
}

// Returns true if there is a current sprite to save.
// [main thread]
bool SaveFileCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
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

    save_document_in_background(documentWriter, true);
    update_screen_for_document(documentWriter);
  }
  // If the document isn't associated to a file, we must to show the
  // save-as dialog to the user to select for first time the file-name
  // for this document.
  else {
    save_as_dialog(reader, "Save File", true);
  }
}

class SaveFileAsCommand : public Command {
public:
  SaveFileAsCommand();
  Command* clone() { return new SaveFileAsCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

SaveFileAsCommand::SaveFileAsCommand()
  : Command("SaveFileAs",
            "Save File As",
            CmdRecordableFlag)
{
}

bool SaveFileAsCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void SaveFileAsCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  save_as_dialog(reader, "Save As", true);
}

class SaveFileCopyAsCommand : public Command {
public:
  SaveFileCopyAsCommand();
  Command* clone() { return new SaveFileCopyAsCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

SaveFileCopyAsCommand::SaveFileCopyAsCommand()
  : Command("SaveFileCopyAs",
            "Save File Copy As",
            CmdRecordableFlag)
{
}

bool SaveFileCopyAsCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void SaveFileCopyAsCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Document* document(reader.document());
  base::string old_filename = document->getFilename();

  // show "Save As" dialog
  save_as_dialog(reader, "Save Copy As", false);

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
