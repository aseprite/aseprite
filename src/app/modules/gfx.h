// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODULES_GFX_H_INCLUDED
#define APP_MODULES_GFX_H_INCLUDED
#pragma once

#include "app/color.h"
#include "doc/color_mode.h"
#include "gfx/color.h"
#include "gfx/rect.h"
#include "ui/base.h"

namespace os {
  class Surface;
}

namespace ui {
  class Graphics;
}

namespace app {
  using namespace doc;

  void draw_color(ui::Graphics* g,
                  const gfx::Rect& rc,
                  const app::Color& color,
                  const doc::ColorMode colorMode);

  void draw_color_button(ui::Graphics* g,
                         const gfx::Rect& rc,
                         const app::Color& color,
                         const doc::ColorMode colorMode,
                         const bool hot,
                         const bool drag);

  void draw_alpha_slider(ui::Graphics* g,
                         const gfx::Rect& rc,
                         const app::Color& color);

  void draw_alpha_slider(os::Surface* s,
                         const gfx::Rect& rc,
                         const app::Color& color);

} // namespace app

#endif
