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
#include "ui/rect.h"
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

// Save/Restore rectangles from/to the screen.
struct RectTracker
{
  BITMAP* bmp;
  int x1, y1, x2, y2;
  int npixel;
  int *pixel;
};

static void do_rect(BITMAP* bmp, int x1, int y1, int x2, int y2, RectTracker* rt,
                    void (*proc)(BITMAP* bmp, int x, int y, RectTracker* rt))
{
  int x, y, u1, u2, v1, v2;

  if ((x2 < bmp->cl) || (x1 >= bmp->cr) ||
      (y2 < bmp->ct) || (y1 >= bmp->cb))
    return;

  u1 = MID(bmp->cl, x1, bmp->cr-1);
  u2 = MID(x1, x2, bmp->cr-1);

  if ((y1 >= bmp->ct) && (y1 < bmp->cb)) {
    for (x=u1; x<=u2; x++)
      (*proc)(bmp, x, y1, rt);
  }

  if ((y2 >= bmp->ct) && (y2 < bmp->cb)) {
    for (x=u1; x<=u2; x++)
      (*proc)(bmp, x, y2, rt);
  }

  v1 = MID(bmp->ct, y1+1, bmp->cb-1);
  v2 = MID(v1, y2-1, bmp->cb-1);

  if ((x1 >= bmp->cl) && (x1 < bmp->cr)) {
    for (y=v1; y<=v2; y++)
      (*proc)(bmp, x1, y, rt);
  }

  if ((x2 >= bmp->cl) && (x2 < bmp->cr)) {
    for (y=v1; y<=v2; y++)
      (*proc)(bmp, x2, y, rt);
  }
}

static void count_rect(BITMAP* bmp, int x, int y, RectTracker* rt)
{
  rt->npixel++;
}

static void save_rect(BITMAP* bmp, int x, int y, RectTracker* rt)
{
  rt->pixel[rt->npixel++] = getpixel(bmp, x, y);
}

static void restore_rect(BITMAP* bmp, int x, int y, RectTracker* rt)
{
  putpixel(bmp, x, y, rt->pixel[rt->npixel++]);
}

RectTracker* rect_tracker_new(BITMAP* bmp, int x1, int y1, int x2, int y2)
{
  RectTracker* rt;
  int x, y;

  ui::jmouse_hide();

  if (x1 > x2) { x = x1; x1 = x2; x2 = x; }
  if (y1 > y2) { y = y1; y1 = y2; y2 = y; }

  rt = new RectTracker;

  rt->bmp = bmp;
  rt->x1 = x1;
  rt->y1 = y1;
  rt->x2 = x2;
  rt->y2 = y2;

  rt->npixel = 0;
  do_rect(bmp, x1, y1, x2, y2, rt, count_rect);

  if (rt->npixel > 0)
    rt->pixel = new int[rt->npixel];
  else
    rt->pixel = NULL;

  rt->npixel = 0;
  do_rect(bmp, x1, y1, x2, y2, rt, save_rect);

  ui::jmouse_show();

  return rt;
}

void rect_tracker_free(RectTracker* rt)
{
  ui::jmouse_hide();

  rt->npixel = 0;
  do_rect(rt->bmp, rt->x1, rt->y1, rt->x2, rt->y2, rt, restore_rect);

  delete[] rt->pixel;
  delete rt;

  ui::jmouse_show();
}

void rectgrid(BITMAP* bmp, int x1, int y1, int x2, int y2, int w, int h)
{
  if (w < 1 || h < 1)
    return;

  int x, y, u, v, c1, c2;

  c1 = makecol_depth(bitmap_color_depth(bmp), 128, 128, 128);
  c2 = makecol_depth(bitmap_color_depth(bmp), 192, 192, 192);

  u = 0;
  v = 0;
  for (y=y1; y<=y2-h; y+=h) {
    for (x=x1; x<=x2-w; x+=w)
      rectfill(bmp, x, y, x+w-1, y+h-1, ((u++)&1)? c1: c2);

    if (x <= x2)
      rectfill(bmp, x, y, x2, y+h-1, ((u++)&1)? c1: c2);

    u = (++v);
  }

  if (y <= y2) {
    for (x=x1; x<=x2-w; x+=w)
      rectfill(bmp, x, y, x+w-1, y2, ((u++)&1)? c1: c2);

    if (x <= x2)
      rectfill(bmp, x, y, x2, y2, ((u++)&1)? c1: c2);
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

void draw_color(BITMAP* bmp, const Rect& rc, PixelFormat pixelFormat, const app::Color& color)
{
  app::Color::Type type = color.getType();
  BITMAP* graph;

  if (type == app::Color::MaskType) {
    rectgrid(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, rc.w/4, rc.h/2);
    return;
  }
  else if (type == app::Color::IndexType) {
    int index = color.getIndex();

    if (index >= 0 && index < get_current_palette()->size()) {
      rectfill(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1,
               color_utils::color_for_allegro(color, bitmap_color_depth(bmp)));
    }
    else {
      rectfill(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, makecol(0, 0, 0));
      line(bmp, rc.x+rc.w-2, rc.y+1, rc.x+1, rc.y+rc.h-2, makecol(255, 255, 255));
    }
    return;
  }

  switch (pixelFormat) {

    case IMAGE_INDEXED:
      rectfill(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1,
               color_utils::color_for_allegro(app::Color::fromIndex(color_utils::color_for_image(color, pixelFormat)),
                                              bitmap_color_depth(bmp)));
      break;

    case IMAGE_RGB:
      graph = create_bitmap_ex(32, rc.w, rc.h);
      if (!graph)
        return;

      {
        int rgb_bitmap_color = color_utils::color_for_image(color, pixelFormat);
        app::Color color2 = app::Color::fromRgb(rgba_getr(rgb_bitmap_color),
                                                rgba_getg(rgb_bitmap_color),
                                                rgba_getb(rgb_bitmap_color));
        rectfill(graph, 0, 0, rc.w-1, rc.h-1,
                 color_utils::color_for_allegro(color2, 32));
      }

      blit(graph, bmp, 0, 0, rc.x, rc.y, rc.w, rc.h);

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

      blit(graph, bmp, 0, 0, rc.x, rc.y, rc.w, rc.h);

      destroy_bitmap(graph);
      break;
  }
}

void draw_color_button(BITMAP* bmp,
                       const Rect& rc,
                       bool outer_nw, bool outer_n, bool outer_ne, bool outer_e,
                       bool outer_se, bool outer_s, bool outer_sw, bool outer_w,
                       PixelFormat pixelFormat, const app::Color& color, bool hot, bool drag)
{
  SkinTheme* theme = (SkinTheme*)ui::CurrentTheme::get();
  int scale = ui::jguiscale();

  // Draw background (the color)
  draw_color(bmp,
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
    theme->draw_bounds_array(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, parts);
  }

  // Draw hot
  if (hot) {
    theme->draw_bounds_nw(bmp,
                          rc.x, rc.y,
                          rc.x+rc.w-1,
                          rc.y+rc.h-1 - (outer_s ? 1*scale: 0),
                          PART_COLORBAR_BORDER_HOTFG_NW);
  }
}

} // namespace app
