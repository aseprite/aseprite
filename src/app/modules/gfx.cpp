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

#include "ui/color.h"
#include "ui/intern.h"
#include "ui/system.h"
#include "ui/theme.h"

#include "app/app.h"
#include "app/color_utils.h"
#include "app/ui/editor/editor.h"
#include "app/console.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "app/ini_file.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "app/ui/skin/skin_theme.h"

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
  ui::Color c1 = ui::rgba(128, 128, 128);
  ui::Color c2 = ui::rgba(192, 192, 192);

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

void draw_emptyset_symbol(BITMAP* bmp, const Rect& rc, ui::Color color)
{
  Point center = rc.getCenter();
  int size = MIN(rc.w, rc.h) - 8;
  size = MID(4, size, 64);

  circle(bmp, center.x, center.y, size*4/10, ui::to_system(color));
  line(bmp,
       center.x-size/2, center.y+size/2,
       center.x+size/2, center.y-size/2, ui::to_system(color));
}

static void draw_color(ui::Graphics* g, const Rect& rc, PixelFormat pixelFormat, const app::Color& color)
{
  if (rc.w < 1 || rc.h < 1)
    return;

  app::Color::Type type = color.getType();
  BITMAP* graph;

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
      g->fillRect(ui::rgba(0, 0, 0), rc);
      g->drawLine(ui::rgba(255, 255, 255),
        gfx::Point(rc.x+rc.w-2, rc.y+1),
        gfx::Point(rc.x+1, rc.y+rc.h-2));
    }
    return;
  }

  switch (pixelFormat) {

    case IMAGE_INDEXED:
      g->fillRect(
        color_utils::color_for_ui(
          app::Color::fromIndex(
            color_utils::color_for_image(color, pixelFormat))),
        rc);
      break;

    case IMAGE_RGB:
      graph = create_bitmap_ex(32, rc.w, rc.h);
      if (!graph)
        return;

      {
        raster::color_t rgb_bitmap_color = color_utils::color_for_image(color, pixelFormat);
        app::Color color2 = app::Color::fromRgb(rgba_getr(rgb_bitmap_color),
                                                rgba_getg(rgb_bitmap_color),
                                                rgba_getb(rgb_bitmap_color));
        rectfill(graph, 0, 0, rc.w-1, rc.h-1,
                 color_utils::color_for_allegro(color2, 32));
      }

      g->blit(graph, 0, 0, rc.x, rc.y, rc.w, rc.h);

      destroy_bitmap(graph);
      break;

    case IMAGE_GRAYSCALE:
      graph = create_bitmap_ex(32, rc.w, rc.h);
      if (!graph)
        return;

      {
        int gray_bitmap_color = color_utils::color_for_image(color, pixelFormat);
        app::Color color2 = app::Color::fromGray(graya_getv(gray_bitmap_color));
        rectfill(graph, 0, 0, rc.w-1, rc.h-1,
                 color_utils::color_for_allegro(color2, 32));
      }

      g->blit(graph, 0, 0, rc.x, rc.y, rc.w, rc.h);

      destroy_bitmap(graph);
      break;
  }
}

void draw_color_button(ui::Graphics* g,
                       const Rect& rc,
                       bool outer_nw, bool outer_n, bool outer_ne, bool outer_e,
                       bool outer_se, bool outer_s, bool outer_sw, bool outer_w,
                       PixelFormat pixelFormat, const app::Color& color, bool hot, bool drag)
{
  SkinTheme* theme = (SkinTheme*)ui::CurrentTheme::get();
  int scale = ui::jguiscale();

  // Draw background (the color)
  draw_color(g,
    Rect(rc.x+1*scale,
      rc.y+1*scale,
      rc.w-((outer_e) ? 2*scale: 1*scale),
      rc.h-((outer_s) ? 2*scale: 1*scale)), pixelFormat, color);

  // Draw opaque border
  {
    int parts[8] = {
      outer_nw ? PART_COLORBAR_0_NW: PART_COLORBAR_3_NW,
      outer_n  ? PART_COLORBAR_0_N : PART_COLORBAR_2_N,
      outer_ne ? PART_COLORBAR_1_NE: (outer_e ? PART_COLORBAR_3_NE: PART_COLORBAR_2_NE),
      outer_e  ? PART_COLORBAR_1_E : PART_COLORBAR_0_E,
      outer_se ? PART_COLORBAR_3_SE: (outer_s ? PART_COLORBAR_2_SE: (outer_e ? PART_COLORBAR_1_SE: PART_COLORBAR_0_SE)),
      outer_s  ? PART_COLORBAR_2_S : PART_COLORBAR_0_S,
      outer_sw ? PART_COLORBAR_2_SW: (outer_s ? PART_COLORBAR_3_SW: PART_COLORBAR_1_SW),
      outer_w  ? PART_COLORBAR_0_W : PART_COLORBAR_1_W,
    };
    theme->draw_bounds_array(g, rc, parts);
  }

  // Draw hot
  if (hot) {
    theme->draw_bounds_nw(g,
      gfx::Rect(rc.x, rc.y, rc.w, rc.h-1 - (outer_s ? 1*scale: 0)),
      PART_COLORBAR_BORDER_HOTFG_NW);
  }
}

} // namespace app
