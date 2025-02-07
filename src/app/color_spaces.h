// Aseprite
// Copyright (c) 2018-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COLOR_SPACES_H_INCLUDED
#define APP_COLOR_SPACES_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "gfx/color_space.h"
#include "os/color_space.h"

namespace doc {
class Sprite;
}

namespace app {
class Preferences;

void initialize_color_spaces(Preferences& pref);

os::ColorSpaceRef get_screen_color_space();

// Returns the color space of the current document.
os::ColorSpaceRef get_current_color_space();

gfx::ColorSpaceRef get_working_rgb_space_from_preferences();

class ConvertCS {
public:
  ConvertCS();
  ConvertCS(const os::ColorSpaceRef& srcCS, const os::ColorSpaceRef& dstCS);
  ConvertCS(ConvertCS&&);
  ConvertCS& operator=(const ConvertCS&) = delete;
  gfx::Color operator()(const gfx::Color c);

private:
  os::Ref<os::ColorSpaceConversion> m_conversion;
};

ConvertCS convert_from_current_to_screen_color_space();
ConvertCS convert_from_custom_to_srgb(const os::ColorSpaceRef& from);

} // namespace app

#endif
