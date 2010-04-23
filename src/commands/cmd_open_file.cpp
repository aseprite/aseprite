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

#include <stdio.h>
#include <allegro.h>

#include "jinete/jinete.h"

#include "ui_context.h"
#include "commands/command.h"
#include "commands/params.h"
#include "console.h"
#include "app.h"
#include "dialogs/filesel.h"
#include "file/file.h"
#include "raster/sprite.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "recent_files.h"
#include "widgets/statebar.h"

//////////////////////////////////////////////////////////////////////
// open_file

class OpenFileCommand : public Command
{
  std::string m_filename;

public:
  OpenFileCommand();
  Command* clone() { return new OpenFileCommand(*this); }

protected:
  void load_params(Params* params);
  void execute(Context* context);
};

struct OpenFileData
{
  Monitor *monitor;
  FileOp *fop;
  Progress *progress;
  JThread thread;
  Frame* alert_window;
};

/**
 * Thread to do the hard work: load the file from the disk.
 *
 * [loading thread]
 */
static void openfile_bg(void *fop_data)
{
  FileOp* fop = (FileOp*)fop_data;

  try {
    fop_operate(fop);
  }
  catch (const std::exception& e) {
    fop_error(fop, _("Error loading file:\n%s"), e.what());
  }

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
static void monitor_openfile_bg(void* _data)
{
  OpenFileData* data = (OpenFileData*)_data;
  FileOp* fop = (FileOp*)data->fop;

  if (data->progress)
    data->progress->setPos(fop_get_progress(fop));

  // Is done? ...ok, now the sprite is in the main thread only...
  if (fop_is_done(fop))
    remove_gui_monitor(data->monitor);
}

/**
 * Called to destroy the data of the monitor.
 * 
 * [main thread]
 */
static void monitor_free(void* _data)
{
  OpenFileData* data = (OpenFileData*)_data;

  if (data->alert_window != NULL) {
    data->monitor = NULL;
    data->alert_window->closeWindow(NULL);
  }
}

OpenFileCommand::OpenFileCommand()
  : Command("open_file",
	    "Open Sprite",
	    CmdRecordableFlag)
{
  m_filename = "";
}

void OpenFileCommand::load_params(Params* params)
{
  m_filename = params->get("filename");
}

/**
 * Command to open a file.
 *
 * [main thread]
 */
void OpenFileCommand::execute(Context* context)
{
  Console console;

  // interactive
  if (context->is_ui_available() && m_filename.empty()) {
    char exts[4096];
    get_readable_extensions(exts, sizeof(exts));
    m_filename = ase_file_selector(friendly_name(), "", exts);
  }

  if (!m_filename.empty()) {
    FileOp *fop = fop_to_load_sprite(m_filename.c_str(), FILE_LOAD_SEQUENCE_ASK);
    bool unrecent = false;

    if (fop) {
      if (fop->error) {
	console.printf(fop->error);
	fop_free(fop);

	unrecent = true;
      }
      else {
	JThread thread = jthread_new(openfile_bg, fop);
	if (thread) {
	  OpenFileData* data = new OpenFileData;

	  data->fop = fop;
	  data->progress = app_get_statusbar()->addProgress();
	  data->thread = thread;
	  data->alert_window = jalert_new(PACKAGE
					  "<<Loading file:<<%s||&Cancel",
					  get_filename(m_filename.c_str()));

	  // Add a monitor to check the loading (FileOp) progress
	  data->monitor = add_gui_monitor(monitor_openfile_bg,
					  monitor_free, data);

	  data->alert_window->open_window_fg();

	  if (data->monitor != NULL)
	    remove_gui_monitor(data->monitor);

	  // Stop the file-operation and wait the thread to exit
	  fop_stop(data->fop);
	  jthread_join(data->thread);

	  // Show any error
	  if (fop->error)
	    console.printf(fop->error);

	  Sprite *sprite = fop->sprite;
	  if (sprite) {
	    UIContext* context = UIContext::instance();

	    RecentFiles::addRecentFile(fop->filename);
	    context->add_sprite(sprite);

	    set_sprite_in_more_reliable_editor(sprite);
	  }
	  else if (!fop_is_stop(fop))
	    unrecent = true;

	  delete data->progress;
	  jwidget_free(data->alert_window);
	  fop_free(fop);
	  delete data;
	}
	else {
	  console.printf(_("Error creating thread to load the sprite"));
	  fop_free(fop);
	}
      }

      // The file was not found or was loaded loaded with errors,
      // so we can remove it from the recent-file list
      if (unrecent) {
	RecentFiles::removeRecentFile(m_filename.c_str());
      }
    }
    else {
      // Do nothing (the user cancelled or something like that)
    }
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_open_file_command()
{
  return new OpenFileCommand;
}
