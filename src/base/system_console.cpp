// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/system_console.h"

#ifdef _WIN32 // Windows needs some adjustments to the console if the
              // process is linked with /subsystem:windows. These
              // adjustments are not great but are good enough.
              // See system_console.h for more information.

#include <cstdio>
#include <iostream>

#include <windows.h>
#include <io.h>

namespace base {

static bool withConsole = false;

SystemConsole::SystemConsole()
{
  // If some output handle (stdout/stderr) is not attached to a
  // console, we can attach the process to the parent process console.
  bool unknownOut = (::GetFileType(::GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_UNKNOWN);
  bool unknownErr = (::GetFileType(::GetStdHandle(STD_ERROR_HANDLE)) == FILE_TYPE_UNKNOWN);
  if (unknownOut || unknownErr) {
    // AttachConsole() can fails if the parent console doesn't have a
    // console, which is the most common, i.e. when the user
    // double-click a shortcut to start the program.
    if (::AttachConsole(ATTACH_PARENT_PROCESS)) {
      // In this case we're attached to the parent process
      // (e.g. cmd.exe) console.
      withConsole = true;
    }
  }

  if (withConsole) {
    // Here we redirect stdout/stderr to use the parent console's ones.
    if (unknownOut) std::freopen("CONOUT$", "w", stdout);
    if (unknownErr) std::freopen("CONOUT$", "w", stderr);

    // Synchronize C++'s cout/cerr streams with C's stdout/stderr.
    std::ios::sync_with_stdio();
  }
}

SystemConsole::~SystemConsole()
{
  if (withConsole) {
    ::FreeConsole();
    withConsole = false;
  }
}

void SystemConsole::prepareShell()
{
  if (withConsole)
    ::FreeConsole();

  // In this case, for a better user experience, here we create a new
  // console so he can write text in a synchronized way with the
  // console. (The parent console stdin is not reliable for
  // interactive command input in the current state, without doing
  // this the input from the cmd.exe would be executed by cmd.exe and
  // by our app.)
  withConsole = true;
  ::AllocConsole();
  ::AttachConsole(::GetCurrentProcessId());

  std::freopen("CONIN$", "r", stdin);
  std::freopen("CONOUT$", "w", stdout);
  std::freopen("CONOUT$", "w", stderr);
  std::ios::sync_with_stdio();
}

}

#else  // On Unix-like systems the console works just fine

namespace base {

SystemConsole::SystemConsole() { }
SystemConsole::~SystemConsole() { }
void SystemConsole::prepareShell() { }

}

#endif
