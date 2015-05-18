// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_FREEHAND_ALGORITHM_H_INCLUDED
#define APP_TOOLS_FREEHAND_ALGORITHM_H_INCLUDED
#pragma once

namespace app {
namespace tools {

  enum class FreehandAlgorithm {
    DEFAULT = 0,
    REGULAR = 0,
    PIXEL_PERFECT = 1,
    DOTS = 2,
  };

} // namespace tools
} // namespace app

#endif
