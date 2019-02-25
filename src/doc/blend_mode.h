// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BLEND_MODE_H_INCLUDED
#define DOC_BLEND_MODE_H_INCLUDED
#pragma once

#include <string>

namespace doc {

  enum class BlendMode {
    // Special internal/undocumented alpha compositing and blend modes
    UNSPECIFIED     = -1,
    SRC             = -2,
    MERGE           = -3,
    NEG_BW          = -4,       // Negative Black & White
    RED_TINT        = -5,
    BLUE_TINT       = -6,
    DST_OVER        = -7,

    // Aseprite (.ase files) blend modes
    NORMAL          = 0,
    MULTIPLY        = 1,
    SCREEN          = 2,
    OVERLAY         = 3,
    DARKEN          = 4,
    LIGHTEN         = 5,
    COLOR_DODGE     = 6,
    COLOR_BURN      = 7,
    HARD_LIGHT      = 8,
    SOFT_LIGHT      = 9,
    DIFFERENCE      = 10,
    EXCLUSION       = 11,
    HSL_HUE         = 12,
    HSL_SATURATION  = 13,
    HSL_COLOR       = 14,
    HSL_LUMINOSITY  = 15,
    ADDITION        = 16,
    SUBTRACT        = 17,
    DIVIDE          = 18
  };

  std::string blend_mode_to_string(BlendMode blendMode);

} // namespace doc

#endif
