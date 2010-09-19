/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "app.h"
#include "console.h"
#include "dialogs/filesel.h"
#include "file/file.h"
#include "modules/gui.h"
#include "recent_files.h"
#include "raster/sprite.h"
#include "widgets/statebar.h"
#include "sprite_wrappers.h"

typedef struct SaveFileData
{
  Monitor *monitor;
  FileOp *fop;
  Progress *progress;
  JThread thread;
  Frame* alert_window;
} SaveFileData;

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

static void save_sprite_in_background(Sprite* sprite, bool mark_as_saved)
{
  FileOp *fop = fop_to_save_sprite(sprite);
  if (fop) {
    JThread thread = jthread_new(savefile_bg, fop);
    if (thread) {
      SaveFileData* data = new SaveFileData;

      data->fop = fop;
      data->progress = app_get_statusbar()->addProgress();
      data->thread = thread;
      data->alert_window = jalert_new(PACKAGE
				      "<<Saving file:<<%s||&Cancel",
				      get_filename(sprite->getFilename()));

      /* add a monitor to check the saving (FileOp) progress */
      data->monitor = add_gui_monitor(monitor_savefile_bg,
				      monitor_free, data);

      /* TODO error handling */

      data->alert_window->open_window_fg();

      if (data->monitor != NULL)
	remove_gui_monitor(data->monitor);

      /* wait the `savefile_bg' thread */
      jthread_join(data->thread);

      /* show any error */
      if (fop->error) {
	Console console;
	console.printf(fop->error);
      }
      /* no error? */
      else {
	App::instance()->getRecentFiles()->addRecentFile(sprite->getFilename());
	if (mark_as_saved)
	  sprite->markAsSaved();

	app_get_statusbar()
	  ->setStatusText(2000, "File %s, saved.",
			  get_filename(sprite->getFilename()));
      }

      delete data->progress;
      jwidget_free(data->alert_window);
      fop_free(fop);
      delete data;
    }
  }
}

/*********************************************************************/

static void save_as_dialog(Sprite* sprite, const char* dlg_title, bool mark_as_saved)
{
  char exts[4096];
  jstring filename;
  jstring newfilename;
  int ret;

  filename = sprite->getFilename();
  get_writable_extensions(exts, sizeof(exts));

  for (;;) {
    newfilename = ase_file_selector(dlg_title, filename, exts);
    if (newfilename.empty())
      return;

    filename = newfilename;

    /* does the file exist? */
    if (exists(filename.c_str())) {
      /* ask if the user wants overwrite the file? */
      ret = jalert("Warning<<File exists, overwrite it?<<%s||&Yes||&No||&Cancel",
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

  sprite->setFilename(filename.c_str());
  app_realloc_sprite_list();

  save_sprite_in_background(sprite, mark_as_saved);
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
  : Command("save_file",
	    "Save File",
	    CmdRecordableFlag)
{
}

/**
 * Returns true if there is a current sprite to save.
 *
 * [main thread]
 */
bool SaveFileCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

/**
 * Saves the current sprite in a file.
 * 
 * [main thread]
 */
void SaveFileCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  /* if the sprite is associated to a file in the file-system, we can
     save it directly without user interaction */
  if (sprite->isAssociatedToFile()) {
    save_sprite_in_background(sprite, true);
  }
  /* if the sprite isn't associated to a file, we must to show the
     save-as dialog to the user to select for first time the file-name
     for this sprite */
  else {
    save_as_dialog(sprite, "Save Sprite", true);
  }
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
  : Command("save_file_as",
	    "Save File As",
	    CmdRecordableFlag)
{
}

bool SaveFileAsCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void SaveFileAsCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  save_as_dialog(sprite, "Save Sprite As", true);
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
  : Command("save_file_copy_as",
	    "Save File Copy As",
	    CmdRecordableFlag)
{
}

bool SaveFileCopyAsCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void SaveFileCopyAsCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  jstring old_filename = sprite->getFilename();

  // show "Save As" dialog
  save_as_dialog(sprite, "Save Sprite Copy As", false);

  // restore the file name
  sprite->setFilename(old_filename.c_str());
  app_realloc_sprite_list();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_save_file_command()
{
  return new SaveFileCommand;
}

Command* CommandFactory::create_save_file_as_command()
{
  return new SaveFileAsCommand;
}

Command* CommandFactory::create_save_file_copy_as_command()
{
  return new SaveFileCopyAsCommand;
}
