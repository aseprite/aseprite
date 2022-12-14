// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COLOR_UTILS_H_INCLUDED
#define APP_COLOR_UTILS_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/color_target.h"
#include "gfx/color.h"
#include "doc/color.h"
#include "doc/pixel_format.h"

namespace doc {
  class Layer;
}

namespace app {
  namespace color_utils {

    gfx::Color blackandwhite(gfx::Color color);
    gfx::Color blackandwhite_neg(gfx::Color color);

    app::Color color_from_ui(const gfx::Color color);
    gfx::Color color_for_ui(const app::Color& color);
    doc::color_t color_for_image(const app::Color& color, doc::PixelFormat format);
    doc::color_t color_for_image_without_alpha(const app::Color& color, doc::PixelFormat format);
    doc::color_t color_for_layer(const app::Color& color, doc::Layer* layer);
    doc::color_t color_for_target_mask(const app::Color& color, const ColorTarget& colorTarget);
    doc::color_t color_for_target(const app::Color& color, const ColorTarget& colorTarget);

  } // namespace color_utils
} // namespace app

#endif
