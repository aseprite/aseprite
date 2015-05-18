// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_SELECTION_MODE_H_INCLUDED
#define APP_TOOLS_SELECTION_MODE_H_INCLUDED
#pragma once

namespace app {
namespace tools {

  enum class SelectionMode {
    DEFAULT = 0,
    ADD = 1,
    SUBTRACT = 2,
  };

} // namespace tools
} // namespace app

#endif
