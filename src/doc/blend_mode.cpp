// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/blend_mode.h"

namespace doc {

std::string blend_mode_to_string(BlendMode blendMode)
{
  switch (blendMode) {
    case BlendMode::NORMAL:         return "normal";
    case BlendMode::MULTIPLY:       return "multiply";
    case BlendMode::SCREEN:         return "screen";
    case BlendMode::OVERLAY:        return "overlay";
    case BlendMode::DARKEN:         return "darken";
    case BlendMode::LIGHTEN:        return "lighten";
    case BlendMode::COLOR_DODGE:    return "color_dodge";
    case BlendMode::COLOR_BURN:     return "color_burn";
    case BlendMode::HARD_LIGHT:     return "hard_light";
    case BlendMode::SOFT_LIGHT:     return "soft_light";
    case BlendMode::DIFFERENCE:     return "difference";
    case BlendMode::EXCLUSION:      return "exclusion";
    case BlendMode::HSL_HUE:        return "hsl_hue";
    case BlendMode::HSL_SATURATION: return "hsl_saturation";
    case BlendMode::HSL_COLOR:      return "hsl_color";
    case BlendMode::HSL_LUMINOSITY: return "hsl_luminosity";
    case BlendMode::ADDITION:       return "addition";
    case BlendMode::SUBTRACT:       return "subtract";
    case BlendMode::DIVIDE:         return "divide";
    default:                        return "unknown";
  }
}

} // namespace doc
