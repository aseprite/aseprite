// Aseprite
// Copyright (c) 2018-2025  Igara Studio S.A.
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

namespace ui {
class Display;
}

namespace app {
class Doc;
class Preferences;

void initialize_color_spaces(Preferences& pref);

// Returns the color space of the current document.
os::ColorSpaceRef get_current_color_space(ui::Display* display, Doc* doc = nullptr);

gfx::ColorSpaceRef get_working_rgb_space_from_preferences();

class ConvertCS {
public:
  ConvertCS() = delete;
  ConvertCS(ui::Display* display, Doc* doc = nullptr);
  ConvertCS(const os::ColorSpaceRef& srcCS, const os::ColorSpaceRef& dstCS);
  ConvertCS(ConvertCS&&);
  ConvertCS& operator=(const ConvertCS&) = delete;
  gfx::Color operator()(const gfx::Color c);

private:
  os::Ref<os::ColorSpaceConversion> m_conversion;
};

ConvertCS convert_from_current_to_display_color_space(ui::Display* display);
ConvertCS convert_from_custom_to_srgb(const os::ColorSpaceRef& from);

} // namespace app

#endif
