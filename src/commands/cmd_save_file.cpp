/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include <allegro.h>

#include "app.h"
#include "base/thread.h"
#include "commands/command.h"
#include "console.h"
#include "dialogs/filesel.h"
#include "file/file.h"
#include "gui/gui.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "recent_files.h"
#include "document_wrappers.h"
#include "widgets/statebar.h"

struct SaveFileData
{
  Monitor *monitor;
  FileOp *fop;
  Progress *progress;
  AlertPtr alert_window;
};

/**
 * Thread to do the hard work: save the file to the disk.
 *
 * [saving thread]
 */
static void savefile_bg(void *fop_data)
{
  FileOp *fop = (FileOp *)fop_data;
  try {
    fop_operate(fop);
  }
  catch (const std::exception& e) {
    fop_error(fop, "Error saving file:\n%s", e.what());
  }
  fop_done(fop);
}

/**
 * Called by the gui-monitor (a timer in the gui module that is called
 * every 100 milliseconds).
 * 
 * [main thread]
 */
static void monitor_savefile_bg(void *_data)
{
  SaveFileData *data = (SaveFileData *)_data;
  FileOp *fop = (FileOp *)data->fop;

  if (data->progress)
    data->progress->setPos(fop_get_progress(fop));

  if (fop_is_done(fop))
    remove_gui_monitor(data->monitor);
}

/**
 * Called when the monitor is destroyed.
 * 
 * [main thread]
 */
static void monitor_free(void *_data)
{
  SaveFileData *data = (SaveFileData*)_data;

  if (data->alert_window != NULL) {
    data->monitor = NULL;
    data->alert_window->closeWindow(NULL);
  }
}

static void save_document_in_background(Document* document, bool mark_as_saved)
{
  FileOp *fop = fop_to_save_document(document);
  if (!fop)
    return;

  base::thread thread(&savefile_bg, fop);
  SaveFileData* data = new SaveFileData;

  data->fop = fop;
  data->progress = app_get_statusbar()->addProgress();
  data->alert_window = Alert::create(PACKAGE
				     "<<Saving file:<<%s||&Cancel",
				     get_filename(document->getFilename()));

  /* add a monitor to check the saving (FileOp) progress */
  data->monitor = add_gui_monitor(monitor_savefile_bg,
				  monitor_free, data);

  /* TODO error handling */

  data->alert_window->open_window_fg();

  if (data->monitor != NULL)
    remove_gui_monitor(data->monitor);

  /* wait the `savefile_bg' thread */
  thread.join();

  /* show any error */
  if (fop->has_error()) {
    Console console;
    console.printf(fop->error.c_str());
  }
  /* no error? */
  else {
    App::instance()->getRecentFiles()->addRecentFile(document->getFilename());
    if (mark_as_saved)
      document->markAsSaved();

    app_get_statusbar()
      ->setStatusText(2000, "File %s, saved.",
		      get_filename(document->getFilename()));
  }

  delete data->progress;
  fop_free(fop);
  delete data;
}

/*********************************************************************/

static void save_as_dialog(const DocumentReader& document, const char* dlg_title, bool mark_as_saved)
{
  char exts[4096];
  base::string filename;
  base::string newfilename;
  int ret;

  filename = document->getFilename();
  get_writable_extensions(exts, sizeof(exts));

  for (;;) {
    newfilename = ase_file_selector(dlg_title, filename, exts);
    if (newfilename.empty())
      return;

    filename = newfilename;

    /* does the file exist? */
    if (exists(filename.c_str())) {
      /* ask if the user wants overwrite the file? */
      ret = Alert::show("Warning<<File exists, overwrite it?<<%s||&Yes||&No||&Cancel",
			get_filename(filename.c_str()));
    }
    else
      break;

    /* "yes": we must continue with the operation... */
    if (ret == 1)
      break;
    /* "cancel" or <esc> per example: we back doing nothing */
    else if (ret != 2)
      return;

    /* "no": we must back to select other file-name */
  }

  {
    DocumentWriter documentWriter(document);

    // Change the document file name
    documentWriter->setFilename(filename.c_str());

    // Save the document
    save_document_in_background(documentWriter, mark_as_saved);
  }
}

//////////////////////////////////////////////////////////////////////
// save_file

class SaveFileCommand : public Command
{
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
  const ActiveDocumentReader document(context);

  // If the document is associated to a file in the file-system, we can
  // save it directly without user interaction.
  if (document->isAssociatedToFile()) {
    DocumentWriter documentWriter(document);

    save_document_in_background(documentWriter, true);
  }
  // If the document isn't associated to a file, we must to show the
  // save-as dialog to the user to select for first time the file-name
  // for this document.
  else {
    save_as_dialog(document, "Save File", true);
  }

  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// save_file_as

class SaveFileAsCommand : public Command
{
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
  const ActiveDocumentReader document(context);
  save_as_dialog(document, "Save As", true);
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// save_file_copy_as

class SaveFileCopyAsCommand : public Command
{
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
  const ActiveDocumentReader document(context);
  base::string old_filename = document->getFilename();

  // show "Save As" dialog
  save_as_dialog(document, "Save Copy As", false);

  // Restore the file name.
  {
    DocumentWriter documentWriter(document);
    documentWriter->setFilename(old_filename.c_str());
  }

  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

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
