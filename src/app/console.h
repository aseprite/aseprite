// Aseprite
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

    bool hasText() const;

    void printf(const char *format, ...);

    static void showException(const std::exception& e);

  private:
    bool m_withUI;
  };

} // namespace app

#endif
