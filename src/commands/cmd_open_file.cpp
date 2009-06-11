/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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
#include <allegro.h>

#include "jinete/jinete.h"

#include "ase/ui_context.h"
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
  JWidget alert_window;
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
    delete fop->sprite;
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
#if 0
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
      Console console;
      console.printf(fop->error);
    }
#endif

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

#if 0
  /* stop the file-operation and wait the thread to exit */
  fop_stop(fop);
  jthread_join(data->thread);
#endif

  if (data->alert_window != NULL) {
    data->monitor = NULL;
    jwindow_close(data->alert_window, NULL);
  }

#if 0
  fop_free(fop);
  jfree(data);
#endif
}

/**
 * Command to open a file.
 *
 * [main thread]
 */
static void cmd_open_file_execute(const char *argument)
{
  Console console;
  jstring filename;

  /* interactive */
  if (!argument) {
    char exts[4096];
    get_readable_extensions(exts, sizeof(exts));
    filename = ase_file_selector(_("Open Sprite"), "", exts);
  }
  /* load the file specified in the argument */
  else {
    filename = argument;
  }

  if (!filename.empty()) {
    FileOp *fop = fop_to_load_sprite(filename.c_str(), FILE_LOAD_SEQUENCE_ASK);

    if (fop) {
      if (fop->error) {
	console.printf(fop->error);
	fop_free(fop);
      }
      else {
	JThread thread = jthread_new(openfile_bg, fop);
	if (thread) {
	  OpenFileData* data = new OpenFileData;

	  data->fop = fop;
	  data->progress = progress_new(app_get_statusbar());
	  data->thread = thread;
	  data->alert_window = jalert_new(PACKAGE
					  "<<Loading file:<<%s||&Cancel",
					  get_filename(filename.c_str()));

	  /* add a monitor to check the loading (FileOp) progress */
	  data->monitor = add_gui_monitor(monitor_openfile_bg,
					  monitor_free, data);

	  jwindow_open_fg(data->alert_window);

	  if (data->monitor != NULL)
	    remove_gui_monitor(data->monitor);

	  /* stop the file-operation and wait the thread to exit */
	  fop_stop(data->fop);
	  jthread_join(data->thread);

	  /* show any error */
	  if (fop->error) {
	    console.printf(fop->error);
	  }
	  else {
	    Sprite *sprite = fop->sprite;
	    if (sprite) {
	      UIContext* context = UIContext::instance();

	      recent_file(fop->filename);
	      context->add_sprite(sprite);
	      context->show_sprite(sprite);
	    }
	    /* if the sprite isn't NULL and the file-operation wasn't
	       stopped by the user...  */
	    else if (!fop_is_stop(fop)) {
	      /* ...the file can't be loaded by errors, so we can remove it
		 from the recent-file list */
	      unrecent_file(fop->filename);
	    }
	  }

	  progress_free(data->progress);
	  jwidget_free(data->alert_window);
	  fop_free(fop);
	  delete data;
	}
	else {
	  console.printf(_("Error creating thread to load the sprite"));
	  fop_free(fop);
	}
      }
    }
    else {
      /* do nothing (the user cancelled or something like that) */
    }
  }
}

Command cmd_open_file = {
  CMD_OPEN_FILE,
  NULL,
  NULL,
  cmd_open_file_execute,
  NULL
};
