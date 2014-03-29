// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_SYSTEM_CONSOLE_H_INCLUDED
#define BASE_SYSTEM_CONSOLE_H_INCLUDED
#pragma once

namespace base {

// This class is needed only for Windows platform.
//
// Some background: This app is linked with /subsystem:windows flag,
// which is the only way to avoid a console when the program is
// double-clicked from Windows Explorer. The problem with this is if
// the user starts the program from cmd.exe, the output is not shown
// anywhere. Generally there is one simple solution for console only
// apps: The /subsystem:console flag, but it shows a console when the
// program is started from Windows Explorer (anyway we could call
// FreeConsole(), but the console is visible some milliseconds which
// is not an expected behavior by normal Windows user).
//
// So this class tries to make some adjustments for apps linked with
// /subsystem:windows but that want to display some text in the
// console in certain cases (e.g. when --help flag is specified).
//
// Basically it tries to redirect stdin/stdout handles so the end-user
// can have the best of both worlds on Windows:
// 1) If the app is executed with double-click, a console isn't shown.
//    (Because the process was/should be linked with /subsystem:windows flag.)
// 2) If the app is executed from a process like cmd.exe, the output is
//    redirected to the cmd.exe console.
//
// In the best case, the application should work as a Unix-like
// program, blocking the cmd.exe in case 2, but it cannot be
// possible. The output/input is deattached as soon as cmd.exe knows
// that the program was linked with /subsystem:windows.
//
class SystemConsole
{
public:
  SystemConsole();
  ~SystemConsole();

  // On Windows it creates a console so the user can start typing
  // commands on it. It's necessary because we link the program with
  // /subsystem:windows flag (not /subsystem:console), so the
  // process's stdin starts deattached from the parent process's
  // console. (On Unix-like systems it does nothing.)
  void prepareShell();
};

} // namespace base

#endif
