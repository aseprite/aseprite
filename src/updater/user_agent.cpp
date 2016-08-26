// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <sstream>

#if _WIN32 // Windows

  #include <windows.h>

  extern "C" BOOL IsWow64();

#elif __APPLE__ // Mac OS X

  void getMacOSXVersion(int* major, int* minor, int* bugFix);

#else  // Unix-like system

  #include <sys/utsname.h>

#endif

namespace updater {

std::string getUserAgent()
{
  std::stringstream userAgent;

  // App name and version

#ifndef PACKAGE
#define PACKAGE ""
#pragma message("warning: Define PACKAGE macro")
#endif

#ifndef VERSION
#define VERSION ""
#pragma message("warning: Define VERSION macro")
#endif

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
