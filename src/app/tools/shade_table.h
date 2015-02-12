// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_SHADE_TABLE_H_INCLUDED
#define APP_TOOLS_SHADE_TABLE_H_INCLUDED
#pragma once

#include "app/tools/shading_mode.h"
#include <vector>

namespace app {
  class ColorSwatches;

  namespace tools {

    // Converts a ColorSwatches table to a temporary "shade table" used in
    // shading ink so we can quickly rotate colors with left/right mouse
    // buttons.
    class ShadeTable8 {
    public:
      ShadeTable8(const app::ColorSwatches& colorSwatches, ShadingMode mode);

      uint8_t left(uint8_t index) { return m_left[index]; }
      uint8_t right(uint8_t index) { return m_right[index]; }

    private:
      std::vector<uint8_t> m_left;
      std::vector<uint8_t> m_right;
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_SHADE_TABLE_H_INCLUDED
