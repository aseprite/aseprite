// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "updater/user_agent.h"
#include "ver/info.h"

#include <string>
#include <sstream>

#if _WIN32 // Windows

  #include <windows.h>

  extern "C" BOOL IsWow64();
  extern "C" const char* WineVersion();

#elif __APPLE__ // Mac OS X

  void getMacOSXVersion(int& major, int& minor, int& patch);

#else  // Unix-like system

  #include "base/trim_string.h"
  #include "cfg/cfg.h"
  #include <cstring>
  #include <sys/utsname.h>

#endif

namespace updater {

std::string getUserAgent()
{
  std::stringstream userAgent;

  // App name and version
  userAgent << get_app_name() << "/" << get_app_version() << " (";

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

  if (const char* wineVer = WineVersion())
    userAgent << "; Wine " << wineVer;

#elif __APPLE__

  // ----------------------------------------------------------------------
  // Mac OS X

  int major, minor, patch;
  getMacOSXVersion(major, minor, patch);
  userAgent << "Mac OS X " << major << "." << minor << "." << patch;

#else

  // ----------------------------------------------------------------------
  // Unix like

  cfg::CfgFile file;

  auto isQuote = [](int c) {
    return
    c == '"' ||
    c == '\'' ||
    std::isspace(c);
  };

  auto getValue = [&file, &isQuote](const char* varName) -> std::string {
    std::string result;
    const char* value = file.getValue("", varName, nullptr);
    if (value && std::strlen(value) > 0)
      base::trim_string(value, result, isQuote);
    return result;
  };

  // Read information from /etc/os-release
  file.load("/etc/os-release");
  std::string name = getValue("PRETTY_NAME");
  if (!name.empty()) {
    userAgent << name;
  }
  else {
    name = getValue("NAME");
    std::string version = getValue("VERSION");
    if (!name.empty() && !version.empty()) {
      userAgent << name << " " << version;
    }
    else {
      // Read information from /etc/lsb-release
      file.load("/etc/lsb-release");
      name = getValue("DISTRIB_DESCRIPTION");
      if (!name.empty()) {
        userAgent << name;
      }
      else {
        name = getValue("DISTRIB_ID");
        version = getValue("DISTRIB_RELEASE");
        if (!name.empty() &&
            !version.empty()) {
          userAgent << name << " " << version;
        }
        else {
          // Last resource, use uname() function
          struct utsname utsn;
          uname(&utsn);
          userAgent << utsn.sysname << " " << utsn.release;
        }
      }
    }
  }

#endif

  userAgent << ")";
  return userAgent.str();
}

} // namespace updater
