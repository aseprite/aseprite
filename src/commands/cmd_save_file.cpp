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
#include "modules/recent.h"
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
  fop_operate(fop);
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
    progress_update(data->progress, fop_get_progress(fop));

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
      data->progress = progress_new(app_get_statusbar());
      data->thread = thread;
      data->alert_window = jalert_new(PACKAGE
				      "<<Saving file:<<%s||&Cancel",
				      get_filename(sprite->filename));

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
	recent_file(sprite->filename);
	if (mark_as_saved)
	  sprite_mark_as_saved(sprite);

	statusbar_set_text(app_get_statusbar(),
			   2000, "File %s, saved.",
			   get_filename(sprite->filename));
      }

      progress_free(data->progress);
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

  filename = sprite->filename;
  get_writable_extensions(exts, sizeof(exts));

  for (;;) {
    newfilename = ase_file_selector(dlg_title, filename, exts);
    if (newfilename.empty())
      return;

    filename = newfilename;

    /* does the file exist? */
    if (exists(filename.c_str())) {
      /* ask if the user wants overwrite the file? */
      ret = jalert("%s<<%s<<%s||%s||%s||%s",
		   _("Warning"),
		   _("File exists, overwrite it?"),
		   get_filename(filename.c_str()),
		   _("&Yes"), _("&No"), _("&Cancel"));
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

  sprite_set_filename(sprite, filename.c_str());
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
  bool enabled(Context* context);
  void execute(Context* context);
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
bool SaveFileCommand::enabled(Context* context)
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
void SaveFileCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  /* if the sprite is associated to a file in the file-system, we can
     save it directly without user interaction */
  if (sprite_is_associated_to_file(sprite)) {
    save_sprite_in_background(sprite, true);
  }
  /* if the sprite isn't associated to a file, we must to show the
     save-as dialog to the user to select for first time the file-name
     for this sprite */
  else {
    save_as_dialog(sprite, _("Save Sprite"), true);
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
  bool enabled(Context* context);
  void execute(Context* context);
};

SaveFileAsCommand::SaveFileAsCommand()
  : Command("save_file_as",
	    "Save File As",
	    CmdRecordableFlag)
{
}

bool SaveFileAsCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void SaveFileAsCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  save_as_dialog(sprite, _("Save Sprite As"), true);
}

//////////////////////////////////////////////////////////////////////
// save_file_copy_as

class SaveFileCopyAsCommand : public Command
{
public:
  SaveFileCopyAsCommand();
  Command* clone() { return new SaveFileCopyAsCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

SaveFileCopyAsCommand::SaveFileCopyAsCommand()
  : Command("save_file_copy_as",
	    "Save File Copy As",
	    CmdRecordableFlag)
{
}

bool SaveFileCopyAsCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void SaveFileCopyAsCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  jstring old_filename = sprite->filename;

  // show "Save As" dialog
  save_as_dialog(sprite, _("Save Sprite Copy As"), false);

  // restore the file name
  sprite_set_filename(sprite, old_filename.c_str());
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
