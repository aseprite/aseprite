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

#include "jinete/jalert.h"
#include "launcher.h"

#if defined ALLEGRO_WINDOWS
#include <windows.h>
#endif

void Launcher::openUrl(const std::string& url)
{
  openFile(url);
}

void Launcher::openFile(const std::string& file)
{
  int ret = -1;

#if defined ALLEGRO_WINDOWS

  ShellExecute(NULL, "open", file.c_str(), NULL, NULL, SW_SHOWNORMAL);
  ret = 0;	   // TODO Use ShellExecuteEx to know the return value

#elif defined ALLEGRO_MACOSX

  ret = system(("open \"" + file + "\"").c_str());

#else  // Linux

  ret = system(("xdg-open \"" + file + "\"").c_str());

#endif

  if (ret != 0)
    jalert(_("Problem<<Cannot open:<<%s||&Close"), file.c_str());
}
