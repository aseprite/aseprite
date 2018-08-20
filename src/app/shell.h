// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SHELL_H_INCLUDED
#define APP_SHELL_H_INCLUDED
#pragma once

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

namespace app {
  namespace script {
    class Engine;
  }

  class Shell {
  public:
    Shell();
    ~Shell();

    void run(script::Engine& engine);
  };

} // namespace app

#endif
