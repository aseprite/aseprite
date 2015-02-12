// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_SHADING_MODE_H_INCLUDED
#define APP_TOOLS_SHADING_MODE_H_INCLUDED
#pragma once

namespace app {
  namespace tools {

    enum ShadingMode {
      // When we reach the left/right side of the table, we stay there.
      kDrainShadingMode,

      // Rotate colors (the leftmost is converted to the rightmost and
      // viceversa).
      kRotateShadingMode
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_SHADING_MODE_H_INCLUDED
