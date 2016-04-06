// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_SCRIPT_H_INCLUDED
#define APP_SCRIPT_H_INCLUDED
#pragma once

#include "script/engine.h"

namespace app {

  class AppScripting : public script::Engine {
  public:
    AppScripting(script::EngineDelegate* delegate);
  };

} // namespace app

#endif
