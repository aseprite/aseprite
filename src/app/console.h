// Aseprite
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CONSOLE_H_INCLUDED
#define APP_CONSOLE_H_INCLUDED
#pragma once

#include <exception>

namespace app {
class Context;

class Console {
public:
  Console(Context* ctx = nullptr);
  ~Console();

  void printf(const char* format, ...);

  static void showException(const std::exception& e);
  static void notifyNewDisplayConfiguration();

private:
  static bool isUIAvailable();

  bool m_withUI;

  class ConsoleWindow;
  static ConsoleWindow* m_console;
};

} // namespace app

#endif
