/* ASEPRITE
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
#include "app/ui/main_window.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "ui/manager.h"
#include "ui/window.h"
#include "app/ui_context.h"

#include <allegro.h>
#include <vector>

#ifdef ALLEGRO_WINDOWS
  #include <winalleg.h>
#endif

#ifdef ALLEGRO_WINDOWS

#ifdef STRICT
  typedef WNDPROC wndproc_t;
#else
  typedef FARPROC wndproc_t;
#endif

namespace app {

static wndproc_t base_wnd_proc = NULL;
static std::vector<base::string>* dropped_files;
static base::mutex* dropped_files_mutex = NULL;

static void subclass_hwnd();
static void unsubclass_hwnd();
static LRESULT CALLBACK ase_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

void install_drop_files()
{
  dropped_files = new std::vector<base::string>();
  dropped_files_mutex = new base::mutex();

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
  if (!base_wnd_proc)           // drop-files hook not installed
    return;

  // If the main window is not the current foreground one. We discard
  // the drop-files event.
  if (ui::Manager::getDefault()->getForegroundWindow() != App::instance()->getMainWindow())
    return;

  base::scoped_lock lock(*dropped_files_mutex);
  if (!dropped_files->empty()) {
    std::vector<base::string> files = *dropped_files;
    dropped_files->clear();

    // open all files

    Command* cmd_open_file =
      CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
    Params params;

    for (std::vector<base::string>::iterator
           it = files.begin(); it != files.end(); ++it) {
      params.set("filename", it->c_str());
      UIContext::instance()->executeCommand(cmd_open_file, &params);
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
  base_wnd_proc = (wndproc_t)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ase_wnd_proc);
}

static void unsubclass_hwnd()
{
  HWND hwnd = win_get_window();

  // restore the old wndproc
  SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)base_wnd_proc);
  base_wnd_proc = NULL;
}

static LRESULT CALLBACK ase_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {

    case WM_DROPFILES:
      {
        base::scoped_lock lock(*dropped_files_mutex);
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
  return ::CallWindowProc(base_wnd_proc, hwnd, msg, wparam, lparam);
}

#else

void install_drop_files() { }
void uninstall_drop_files() { }
void check_for_dropped_files() { }

#endif

} // namespace app
