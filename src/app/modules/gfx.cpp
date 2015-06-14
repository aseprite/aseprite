// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/color.h"
#include "ui/intern.h"
#include "ui/system.h"
#include "ui/theme.h"

#include "app/app.h"
#include "app/color_utils.h"
#include "app/console.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "doc/image.h"
#include "doc/palette.h"

namespace app {

using namespace app::skin;
using namespace gfx;

static void rectgrid(ui::Graphics* g, const gfx::Rect& rc, const gfx::Size& tile)
{
  if (tile.w < 1 || tile.h < 1)
    return;

  int x, y, u, v;
  gfx::Color c1 = gfx::rgba(128, 128, 128);
  gfx::Color c2 = gfx::rgba(192, 192, 192);

  u = 0;
  v = 0;
  for (y=rc.y; y<rc.y2()-tile.h; y+=tile.h) {
    for (x=rc.x; x<rc.x2()-tile.w; x+=tile.w)
      g->fillRect(((u++)&1)? c1: c2, gfx::Rect(x, y, tile.w, tile.h));

    if (x < rc.x2())
      g->fillRect(((u++)&1)? c1: c2, gfx::Rect(x, y, rc.x2()-x, tile.h));

    u = (++v);
  }

  if (y < rc.y2()) {
    for (x=rc.x; x<rc.x2()-tile.w; x+=tile.w)
      g->fillRect(((u++)&1)? c1: c2, gfx::Rect(x, y, tile.w, rc.y2()-y));

    if (x < rc.x2())
      g->fillRect(((u++)&1)? c1: c2, gfx::Rect(x, y, rc.x2()-x, rc.y2()-y));
  }
}

static void draw_color(ui::Graphics* g, const Rect& rc, const app::Color& color)
{
  if (rc.w < 1 || rc.h < 1)
    return;

  app::Color::Type type = color.getType();

  if (type == app::Color::MaskType) {
    rectgrid(g, rc, gfx::Size(rc.w/4, rc.h/2));
    return;
  }
  else if (type == app::Color::IndexType) {
    int index = color.getIndex();

    if (index >= 0 && index < get_current_palette()->size()) {
      g->fillRect(color_utils::color_for_ui(color), rc);
    }
    else {
      g->fillRect(gfx::rgba(0, 0, 0), rc);
      g->drawLine(gfx::rgba(255, 255, 255),
        gfx::Point(rc.x+rc.w-2, rc.y+1),
        gfx::Point(rc.x+1, rc.y+rc.h-2));
    }
    return;
  }

  g->fillRect(color_utils::color_for_ui(color), rc);
}

void draw_color_button(ui::Graphics* g,
  const Rect& rc, const app::Color& color,
  bool hot, bool drag)
{
  SkinTheme* theme = (SkinTheme*)ui::CurrentTheme::get();
  int scale = ui::guiscale();

  // Draw background (the color)
  draw_color(g,
    Rect(rc.x+1*scale,
      rc.y+1*scale,
      rc.w-2*scale,
      rc.h-2*scale), color);

  // Draw opaque border
  {
    int parts[8] = {
      PART_COLORBAR_0_NW,
      PART_COLORBAR_0_N,
      PART_COLORBAR_1_NE,
      PART_COLORBAR_1_E,
      PART_COLORBAR_3_SE,
      PART_COLORBAR_2_S,
      PART_COLORBAR_2_SW,
      PART_COLORBAR_0_W
    };
    theme->draw_bounds_array(g, rc, parts);
  }

  // Draw hot
  if (hot) {
    theme->draw_bounds_nw(g,
      gfx::Rect(rc.x, rc.y, rc.w, rc.h-1 - 1*scale),
      PART_COLORBAR_BORDER_HOTFG_NW);
  }
}

} // namespace app
