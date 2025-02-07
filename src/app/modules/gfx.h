// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MODULES_GFX_H_INCLUDED
#define APP_MODULES_GFX_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/pref/preferences.h"
#include "doc/color_mode.h"
#include "gfx/color.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "ui/base.h"

namespace os {
class Surface;
}

namespace ui {
class Graphics;
}

namespace app {
class Site;

gfx::Color grid_color1();
gfx::Color grid_color2();

void draw_checkered_grid(ui::Graphics* g, const gfx::Rect& rc, const gfx::Size& tile);

void draw_checkered_grid(ui::Graphics* g,
                         const gfx::Rect& rc,
                         const gfx::Size& tile,
                         DocumentPreferences& docPref);

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

void draw_tile(ui::Graphics* g, const gfx::Rect& rc, const Site& site, const doc::tile_t tile);

void draw_tile_button(ui::Graphics* g,
                      const gfx::Rect& rc,
                      const Site& site,
                      doc::tile_t tile,
                      const bool hot,
                      const bool drag);

void draw_alpha_slider(ui::Graphics* g, const gfx::Rect& rc, const app::Color& color);

void draw_alpha_slider(os::Surface* s, const gfx::Rect& rc, const app::Color& color);

} // namespace app

#endif
