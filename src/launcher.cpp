/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "base/exception.h"
#include "gui/alert.h"
#include "launcher.h"

#if defined ALLEGRO_WINDOWS
#define BITMAP WINDOWS_BITMAP
#include <windows.h>

static void win32_shell_execute(const char* verb, const char* file, const char* params)
{
  SHELLEXECUTEINFO sh;
  ZeroMemory((LPVOID)&sh, sizeof(sh));
  sh.cbSize = sizeof(sh);
  sh.fMask = SEE_MASK_DEFAULT;
  sh.lpVerb = verb;
  sh.lpFile = file;
  sh.lpParameters = params;
  sh.nShow = SW_SHOWNORMAL;

  if (!ShellExecuteEx(&sh)) {
    int ret = GetLastError();
    if (ret != 0) {
      DWORD flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS;
      LPSTR msgbuf;

      if (FormatMessageA(flags, NULL, ret,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         reinterpret_cast<LPSTR>(&msgbuf),
                         0, NULL)) {
        Alert::show("Problem<<Cannot open:<<%s<<%s||&Close", file, msgbuf);
        LocalFree(msgbuf);

        ret = 0;
      }
    }
  }
}

#endif

void Launcher::openUrl(const std::string& url)
{
  openFile(url);
}

void Launcher::openFile(const std::string& file)
{
  int ret = -1;

#if defined ALLEGRO_WINDOWS

  win32_shell_execute("open", file.c_str(), NULL);
  ret = 0;

#elif defined ALLEGRO_MACOSX

  ret = system(("open \"" + file + "\"").c_str());

#else  // Linux

  ret = system(("xdg-open \"" + file + "\"").c_str());

#endif

  if (ret != 0)
    Alert::show("Problem<<Cannot open:<<%s||&Close", file.c_str());
}

void Launcher::openFolder(const std::string& file)
{
#if defined ALLEGRO_WINDOWS

  win32_shell_execute(NULL, "explorer", ("/e,/select,\"" + file + "\"").c_str());

#elif defined ALLEGRO_MACOSX

  Alert::show("Problem<<This command is not supported on Mac OS X||&Close");

#else  // Linux

  Alert::show("Problem<<This command is not supported on Linux||&Close");

#endif
}
