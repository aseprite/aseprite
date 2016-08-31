// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SHELL_H_INCLUDED
#define APP_SHELL_H_INCLUDED
#pragma once

namespace script {
  class Engine;
}

namespace app {

  class Shell {
  public:
    Shell();
    ~Shell();

    void run(script::Engine& engine);
  };

} // namespace app

#endif
