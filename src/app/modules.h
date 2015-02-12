// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_MODULES_H_INCLUDED
#define APP_MODULES_H_INCLUDED
#pragma once

#include "ui/base.h"

#define REQUIRE_INTERFACE    1

namespace app {

  /**
   * Class to install and uninstall old modules.
   *
   * Legacy modules are programmed in C code and should be refactored to
   * C++ classes.
   */
  class LegacyModules {
  public:
    LegacyModules(int requirements);
    ~LegacyModules();
  };

} // namespace app

#endif
