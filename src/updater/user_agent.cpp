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

#include <sstream>
#include <string>

namespace updater {

std::string getFullOSString()
{
  base::Platform p = base::get_platform();
  std::stringstream os;

#if LAF_WINDOWS

  // ----------------------------------------------------------------------
  // Windows

  os << "Windows";
  switch (p.windowsType) {
    case base::Platform::WindowsType::Server: os << " Server"; break;
    case base::Platform::WindowsType::NT:     os << " NT"; break;
  }
  os << " " << p.osVer.str();

  if (p.servicePack.major() > 0)
    os << " SP" << p.servicePack.major();

  if (p.isWow64)
    os << "; WOW64";

  if (p.wineVer)
    os << "; Wine " << p.wineVer;

#elif LAF_MACOS

  os << "macOS " << p.osVer.major() << "." << p.osVer.minor() << "." << p.osVer.patch();

#else

  // ----------------------------------------------------------------------
  // Unix like

  if (!p.distroName.empty()) {
    os << p.distroName;
    if (!p.distroVer.empty())
      os << " " << p.distroVer;
  }

#endif

  return os.str();
}

std::string getUserAgent()
{
  std::stringstream userAgent;

  // App name and version
  userAgent << get_app_name() << "/" << get_app_version() << " (" << getFullOSString() << ")";
  return userAgent.str();
}

} // namespace updater
