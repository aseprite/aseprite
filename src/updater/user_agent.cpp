/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include <string>
#include <sstream>

#if _WIN32 // Windows

  #include <windows.h>

  extern "C" BOOL IsWow64();

#elif __APPLE__ // Mac OS X

  extern "C" void getMacOSXVersion(int* major, int* minor, int* bugFix);

#else  // Unix-like system

  #include <sys/utsname.h>

#endif

namespace updater {

std::string getUserAgent()
{
  std::stringstream userAgent;

  // ASEPRITE name and version

  userAgent << PACKAGE << "/" << VERSION << " (";

#if _WIN32

  // ----------------------------------------------------------------------
  // Windows

  OSVERSIONINFOEX osv;
  osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  ::GetVersionEx((OSVERSIONINFO*)&osv);

  userAgent << "Windows";
  switch (osv.wProductType) {
    case VER_NT_DOMAIN_CONTROLLER:
    case VER_NT_SERVER:
      userAgent << " Server";
      break;
    case VER_NT_WORKSTATION:
      userAgent << " NT";
      break;
  }
  userAgent << " " << osv.dwMajorVersion << "." << osv.dwMinorVersion;

  if (osv.wServicePackMajor > 0)
    userAgent << " SP" << osv.wServicePackMajor;

  if (IsWow64())
    userAgent << "; WOW64";

#elif __APPLE__

  // ----------------------------------------------------------------------
  // Mac OS X

  int major, minor, bugFix;
  getMacOSXVersion(&major, &minor, &bugFix);
  userAgent << "Mac OS X " << major << "." << minor << "." << bugFix;

#else

  // ----------------------------------------------------------------------
  // Unix like

  struct utsname utsn;
  uname(&utsn);
  userAgent << utsn.sysname << " " << utsn.release;

#endif

  userAgent << ")";
  return userAgent.str();
}

} // namespace updater
