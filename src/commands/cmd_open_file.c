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

#include <stdio.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "dialogs/filesel.h"
#include "file/file.h"
#include "raster/sprite.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/recent.h"
#include "modules/sprites.h"
#include "widgets/statebar.h"

typedef struct OpenFileData
{
  Monitor *monitor;
  FileOp *fop;
  Progress *progress;
  JThread thread;
} OpenFileData;

/**
 * Thread to do the hard work: load the file from the disk.
 *
 * [loading thread]
 */
static void openfile_bg(void *fop_data)
{
  FileOp *fop = (FileOp *)fop_data;

  fop_operate(fop);

  if (fop_is_stop(fop) && fop->sprite) {
    sprite_free(fop->sprite);
    fop->sprite = NULL;
  }

  fop_done(fop);
}

/**
 * Called by the gui-monitor (a timer in the gui module that is called
 * every 100 milliseconds).
 * 
 * [main thread]
 */
static void monitor_openfile_bg(void *_data)
{
  OpenFileData *data = (OpenFileData *)_data;
  FileOp *fop = (FileOp *)data->fop;

  if (data->progress)
    progress_update(data->progress, fop_get_progress(fop));

  /* is done? ...ok, now the sprite is in the main thread only... */
  if (fop_is_done(fop)) {
    Sprite *sprite = fop->sprite;
    if (sprite) {
      recent_file(fop->filename);
      sprite_mount(sprite);
      sprite_show(sprite);
    }
    /* if the sprite isn't NULL and the file-operation wasn't
       stopped by the user...  */
    else if (!fop_is_stop(fop)) {
      /* ...the file can't be loaded by errors, so we can remove it
	 from the recent-file list */
      unrecent_file(fop->filename);
    }

    if (data->progress)
      progress_free(data->progress);

    if (fop->error) {
      console_open();
      console_printf(fop->error);
      console_close();
    }

    remove_gui_monitor(data->monitor);
  }
}

/**
 * Called to destroy the data of the monitor.
 * 
 * [main thread]
 */
static void monitor_free(void *_data)
{
  OpenFileData *data = (OpenFileData *)_data;
  FileOp *fop = (FileOp *)data->fop;

  /* stop the file-operation and wait the thread to exit */
  fop_stop(fop);
  jthread_join(data->thread);

  fop_free(fop);
  jfree(data);
}

/**
 * Command to open a file.
 *
 * [main thread]
 */
static void cmd_open_file_execute(const char *argument)
{
  char *filename;

  /* interactive */
  if (!argument) {
    char exts[4096];
    get_readable_extensions(exts, sizeof(exts));
    filename = ase_file_selector(_("Open Sprite"), "", exts);
  }
  /* load the file specified in the argument */
  else {
    filename = (char *)argument;
  }

  if (filename) {
    FileOp *fop = fop_to_load_sprite(filename, FILE_LOAD_SEQUENCE_ASK);

    if (filename != argument)
      jfree(filename);

    if (fop) {
      if (fop->error) {
	console_printf(fop->error);
	fop_free(fop);
      }
      else {
	JThread thread = jthread_new(openfile_bg, fop);
	if (thread) {
	  OpenFileData *data = jnew(OpenFileData, 1);

	  data->thread = thread;
	  data->fop = fop;

	  /* add the progress bar */
	  if (app_get_statusbar())
	    data->progress = progress_new(app_get_statusbar());
	  else
	    data->progress = NULL;

	  /* add a monitor to check the loading (FileOp) progress */
	  data->monitor = add_gui_monitor(monitor_openfile_bg,
					  monitor_free, data);
	}
	else {
	  console_printf(_("Error creating thread to load the sprite"));
	  fop_free(fop);
	}
      }
    }
    /* else do nothing (the user canceled or something like that) */
  }
}

Command cmd_open_file = {
  CMD_OPEN_FILE,
  NULL,
  NULL,
  cmd_open_file_execute,
  NULL
};
