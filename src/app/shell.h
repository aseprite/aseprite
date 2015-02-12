// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_SHELL_H_INCLUDED
#define APP_SHELL_H_INCLUDED
#pragma once

namespace scripting {
  class Engine;
}

namespace app {

  class Shell {
  public:
    Shell();
    ~Shell();

    void run(scripting::Engine& engine);
  };

} // namespace app

#endif
