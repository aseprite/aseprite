/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "gfx/color.h"
#include "ui/intern.h"
#include "ui/system.h"
#include "ui/theme.h"

#include "app/app.h"
#include "app/color_utils.h"
#include "app/console.h"
#include "app/ini_file.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"

namespace app {

using namespace app::skin;
using namespace gfx;

void dotted_mode(int offset)
{
  /* these pattern were taken from The GIMP:
     gimp-1.3.11/app/display/gimpdisplayshell-marching-ants.h */
  static int pattern_data[8][8] = {
    {
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
    },
    {
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
    },
    {
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
    },
    {
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
    },
    {
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
    },
    {
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
    },
    {
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
    },
    {
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
    },
  };

  static BITMAP* pattern = NULL;
  int x, y, fg, bg;

  if (offset < 0) {
    if (pattern) {
      destroy_bitmap(pattern);
      pattern = NULL;
    }
    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
    return;
  }

  if (!pattern)
    pattern = create_bitmap(8, 8);

  offset = 7 - (offset & 7);

  bg = makecol(0, 0, 0);
  fg = makecol(255, 255, 255);

  clear_bitmap(pattern);

  for (y=0; y<8; y++)
    for (x=0; x<8; x++)
      putpixel(pattern, x, y,
               (pattern_data[offset][y] & (1<<(7-x)))? fg: bg);

  drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
}

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
  int scale = ui::jguiscale();

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
