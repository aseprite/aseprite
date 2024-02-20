// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "updater/user_agent.h"

#include "base/platform.h"
#include "ver/info.h"

#include <string>
#include <sstream>

namespace updater {

std::string getUserAgent()
{
  base::Platform p = base::get_platform();
  std::stringstream userAgent;

  // App name and version
  userAgent << get_app_name() << "/" << get_app_version() << " (";

#if LAF_WINDOWS

  // ----------------------------------------------------------------------
  // Windows

  userAgent << "Windows";
  switch (p.windowsType) {
    case base::Platform::WindowsType::Server:
      userAgent << " Server";
      break;
    case base::Platform::WindowsType::NT:
      userAgent << " NT";
      break;
  }
  userAgent << " " << p.osVer.str();

  if (p.servicePack.major() > 0)
    userAgent << " SP" << p.servicePack.major();

  if (p.isWow64)
    userAgent << "; WOW64";

  if (p.wineVer)
    userAgent << "; Wine " << p.wineVer;

#elif LAF_MACOS

  userAgent << "macOS "
            << p.osVer.major() << "."
            << p.osVer.minor() << "."
            << p.osVer.patch();

#else

  // ----------------------------------------------------------------------
  // Unix like

  if (!p.distroName.empty()) {
    userAgent << p.distroName;
    if (!p.distroVer.empty())
      userAgent << " " << p.distroVer;
  }

#endif

  userAgent << ")";
  return userAgent.str();
}

} // namespace updater
