/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include "commands/commands.h"
#include "core/app.h"
#include "console/console.h"
#include "dialogs/filesel.h"
#include "file/file.h"
#include "modules/gui.h"
#include "modules/recent.h"
#include "modules/sprites.h"
#include "raster/sprite.h"
#include "widgets/statebar.h"

typedef struct SaveFileData
{
  Monitor *monitor;
  FileOp *fop;
  Progress *progress;
  JThread thread;
  JWidget alert_window;
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
  SaveFileData *data = (SaveFileData *)_data;

  if (data->alert_window != NULL) {
    data->monitor = NULL;
    jwindow_close(data->alert_window, NULL);
  }
}

static void save_sprite_in_background(Sprite *sprite)
{
  FileOp *fop = fop_to_save_sprite(sprite);
  if (fop) {
    JThread thread = jthread_new(savefile_bg, fop);
    if (thread) {
      SaveFileData *data = jnew(SaveFileData, 1);

      data->fop = fop;
      data->progress = progress_new(app_get_statusbar());
      data->thread = thread;
      data->alert_window = jalert_new(PACKAGE
				      "<<Saving file:<<%s||&Cancel",
				      get_filename(sprite->filename));
      data->monitor = NULL;

      /* add a monitor to check the saving (FileOp) progress */
      data->monitor = add_gui_monitor(monitor_savefile_bg,
				      monitor_free, data);

      /* TODO error handling */

      jwindow_open_fg(data->alert_window);

      if (data->monitor != NULL)
	remove_gui_monitor(data->monitor);

      /* wait the `savefile_bg' thread */
      jthread_join(data->thread);

      /* show any error */
      if (fop->error) {
	console_open();
	console_printf(fop->error);
	console_close();
      }
      /* no error? */
      else {
	recent_file(sprite->filename);
	sprite_mark_as_saved(sprite);
	statusbar_set_text(app_get_statusbar(),
			   2000, "File %s, saved.",
			   get_filename(sprite->filename));
      }

      progress_free(data->progress);
      jwidget_free(data->alert_window);
      fop_free(fop);
      jfree(data);
    }
  }
}

/*********************************************************************
 Save File
 *********************************************************************/

/**
 * Returns true if there is a current sprite to save.
 *
 * [main thread]
 */
static bool cmd_save_file_enabled(const char *argument)
{
  return current_sprite != NULL;
}

/**
 * Saves the current sprite in a file.
 * 
 * [main thread]
 */
static void cmd_save_file_execute(const char *argument)
{
  Sprite *sprite = current_sprite;

  /* if the sprite is associated to a file in the file-system, we can
     save it directly without user interaction */
  if (sprite_is_associated_to_file(sprite)) {
    save_sprite_in_background(sprite);
  }
  /* if the sprite isn't associated to a file, we must to show the
     save-as dialog to the user to select for first time the file-name
     for this sprite */
  else {
    command_execute(command_get_by_name(CMD_SAVE_FILE_AS), argument);
  }
}

/*********************************************************************
 Save File As
 *********************************************************************/

static bool cmd_save_file_as_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_save_file_as_execute(const char *argument)
{
  Sprite *sprite = current_sprite;
  char filename[4096];
  char exts[4096];
  char *newfilename;
  int ret;

  ustrcpy(filename, sprite->filename);
  get_writable_extensions(exts, sizeof(exts));

  for (;;) {
    newfilename = ase_file_selector(_("Save Sprite"), filename, exts);
    if (!newfilename)
      return;
    ustrcpy(filename, newfilename);
    jfree(newfilename);

    /* does the file exist? */
    if (exists(filename)) {
      /* ask if the user wants overwrite the file? */
      ret = jalert("%s<<%s<<%s||%s||%s||%s",
		   _("Warning"),
		   _("File exists, overwrite it?"),
		   get_filename(filename),
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

  sprite_set_filename(sprite, filename);
  app_realloc_sprite_list();

  save_sprite_in_background(sprite);
}

/**
 * Command to save the current sprite in its associated file.
 */
Command cmd_save_file = {
  CMD_SAVE_FILE,
  cmd_save_file_enabled,
  NULL,
  cmd_save_file_execute,
  NULL
};

/**
 * Command to save the current sprite in another file.
 */
Command cmd_save_file_as = {
  CMD_SAVE_FILE_AS,
  cmd_save_file_as_enabled,
  NULL,
  cmd_save_file_as_execute,
  NULL
};
