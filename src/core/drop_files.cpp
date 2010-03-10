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
#include <vector>

#include "jinete/jstring.h"
#include "jinete/jwindow.h"
#include "Vaca/Mutex.h"
#include "Vaca/ScopedLock.h"

#include "commands/commands.h"
#include "commands/params.h"
#include "app.h"
#include "ui_context.h"

#ifdef ALLEGRO_WINDOWS
  #include <winalleg.h>
#endif

#ifdef ALLEGRO_WINDOWS

using Vaca::Mutex;
using Vaca::ScopedLock;

static WNDPROC base_wnd_proc = NULL;
static std::vector<jstring>* dropped_files;
static Mutex* dropped_files_mutex = NULL;

static void subclass_hwnd();
static void unsubclass_hwnd();
static LRESULT ase_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

void install_drop_files()
{
  dropped_files = new std::vector<jstring>();
  dropped_files_mutex = new Mutex();

  subclass_hwnd();
}

void uninstall_drop_files()
{
  unsubclass_hwnd();
  
  delete dropped_files_mutex;
  dropped_files_mutex = NULL;

  delete dropped_files;
}

void check_for_dropped_files()
{
  if (!base_wnd_proc)		// drop-files hook not installed
    return;

  if (!app_get_top_window()->is_toplevel())
    return;

  ScopedLock lock(*dropped_files_mutex);
  if (!dropped_files->empty()) {
    std::vector<jstring> files = *dropped_files;
    dropped_files->clear();

    // open all files

    Command* cmd_open_file =
      CommandsModule::instance()->get_command_by_name(CommandId::open_file);
    Params params;

    for (std::vector<jstring>::iterator
	   it = files.begin(); it != files.end(); ++it) {
      params.set("filename", it->c_str());
      UIContext::instance()->execute_command(cmd_open_file, &params);
    }
  }
}

static void subclass_hwnd()
{
  HWND hwnd = win_get_window();

  // add the WS_EX_ACCEPTFILES
  SetWindowLong(hwnd, GWL_EXSTYLE,
		GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_ACCEPTFILES);

  // set the GWL_WNDPROC to globalWndProc
  base_wnd_proc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ase_wnd_proc);
}

static void unsubclass_hwnd()
{
  HWND hwnd = win_get_window();

  // restore the old wndproc
  SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)base_wnd_proc);
  base_wnd_proc = NULL;
}

static LRESULT ase_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {

    case WM_DROPFILES:
      {
	ScopedLock lock(*dropped_files_mutex);
	HDROP hdrop = (HDROP)(wparam);
	int index, count, length;

	count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
	for (index=0; index<count; ++index) {
	  length = DragQueryFile(hdrop, index, NULL, 0);
	  if (length > 0) {
	    TCHAR* lpstr = new TCHAR[length+1];
	    DragQueryFile(hdrop, index, lpstr, length+1);
	    dropped_files->push_back(lpstr);
	    delete[] lpstr;
	  }
	}

	DragFinish(hdrop);
      }
      break;

  }
  return (*base_wnd_proc)(hwnd, msg, wparam, lparam);
}

#else

void install_drop_files() { }
void uninstall_drop_files() { }
void check_for_dropped_files() { }

#endif
