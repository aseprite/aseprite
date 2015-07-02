// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_MODULES_GFX_H_INCLUDED
#define APP_MODULES_GFX_H_INCLUDED
#pragma once

#include "app/color.h"
#include "gfx/color.h"
#include "gfx/rect.h"
#include "ui/base.h"
#include "ui/graphics.h"

namespace app {
  using namespace doc;

  void draw_color(ui::Graphics* g,
    const gfx::Rect& rc, const app::Color& color);

  void draw_color_button(ui::Graphics* g,
    const gfx::Rect& rc, const app::Color& color,
    bool hot, bool drag);

} // namespace app

#endif
