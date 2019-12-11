// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COLOR_SPACES_H_INCLUDED
#define APP_COLOR_SPACES_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "gfx/color_space.h"
#include "os/color_space.h"

#include <memory>

namespace doc {
  class Sprite;
}

namespace app {
  class Preferences;

  void initialize_color_spaces(Preferences& pref);

  os::ColorSpacePtr get_screen_color_space();

  // Returns the color space of the current document.
  os::ColorSpacePtr get_current_color_space();

  gfx::ColorSpacePtr get_working_rgb_space_from_preferences();

  class ConvertCS {
  public:
    ConvertCS();
    ConvertCS(const os::ColorSpacePtr& srcCS,
              const os::ColorSpacePtr& dstCS);
    ConvertCS(ConvertCS&&);
    ConvertCS& operator=(const ConvertCS&) = delete;
    gfx::Color operator()(const gfx::Color c);
  private:
    std::unique_ptr<os::ColorSpaceConversion> m_conversion;
  };

  ConvertCS convert_from_current_to_screen_color_space();
  ConvertCS convert_from_custom_to_srgb(const os::ColorSpacePtr& from);

} // namespace app

#endif
