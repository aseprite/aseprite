/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete/jintern.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"

#include "Vaca/Point.h"
#include "Vaca/Rect.h"

#include "app.h"
#include "console.h"
#include "core/cfg.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/skinneable_theme.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "widgets/editor.h"

using Vaca::Rect;
using Vaca::Point;

static BITMAP* gfx_bmps[GFX_BITMAP_COUNT];

#include "modules/gfxdata.cpp"

static void convert_data_to_bitmap(DATA *data, BITMAP** bmp)
{
  int scale = jguiscale();
  const char *p;
  int x, y;
  int black = makecol(0, 0, 0);
  int gray = makecol(128, 128, 128);
  int white = makecol(255, 255, 255);
  int mask;

  *bmp = create_bitmap(data->w * scale,
		       data->h * scale);
  mask = bitmap_mask_color(*bmp);

  p = data->line;
  for (y=0; y<(*bmp)->h; y+=scale) {
    for (x=0; x<(*bmp)->w; x+=scale) {
      rectfill(*bmp, x, y, x+scale-1, y+scale-1,
	       (*p == '#') ? black:
	       (*p == '%') ? gray:
	       (*p == '.') ? white: mask);
      p++;
    }
  }
}

// Slot for App::PaletteChange signal
static void on_palette_change_signal()
{
  for (int c=0; c<GFX_BITMAP_COUNT; c++) {
    if (gfx_bmps[c])
      destroy_bitmap(gfx_bmps[c]);

    gfx_bmps[c] = NULL;
  }
}

int init_module_graphics()
{
  for (int c=0; c<GFX_BITMAP_COUNT; c++)
    gfx_bmps[c] = NULL;

  App::instance()->PaletteChange.connect(&on_palette_change_signal);
  return 0;
}

void exit_module_graphics()
{
  for (int c=0; c<GFX_BITMAP_COUNT; c++)
    if (gfx_bmps[c]) {
      destroy_bitmap(gfx_bmps[c]);
      gfx_bmps[c] = NULL;
    }
}

BITMAP* get_gfx(int id)
{
  if (!gfx_bmps[id])
    convert_data_to_bitmap(&gfx_data[id], &gfx_bmps[id]);

  return gfx_bmps[id];
}

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

void simple_dotted_mode(BITMAP* bmp, int fg, int bg)
{
  static BITMAP* pattern = NULL;

  if (pattern && bitmap_color_depth(pattern) != bitmap_color_depth(bmp))
    destroy_bitmap(pattern);

  pattern = create_bitmap_ex(bitmap_color_depth(bmp), 2, 2);
  clear_bitmap(pattern);

  putpixel(pattern, 0, 0, fg);
  putpixel(pattern, 0, 1, bg);
  putpixel(pattern, 1, 0, bg);
  putpixel(pattern, 1, 1, fg);

  drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
}

/**********************************************************************/
/* Rectangle Tracker (Save/Restore rectangles from/to the screen) */

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

  jmouse_hide();

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

  jmouse_show();

  return rt;
}

void rect_tracker_free(RectTracker* rt)
{
  jmouse_hide();

  rt->npixel = 0;
  do_rect(rt->bmp, rt->x1, rt->y1, rt->x2, rt->y2, rt, restore_rect);

  delete[] rt->pixel;
  delete rt;

  jmouse_show();
}

/**********************************************************************/
/* Rectangles */

void bevel_box(BITMAP* bmp, int x1, int y1, int x2, int y2, int c1, int c2, int bevel)
{
  hline(bmp, x1+bevel, y1, x2-bevel, c1); /* top */
  hline(bmp, x1+bevel, y2, x2-bevel, c2); /* bottom */

  vline(bmp, x1, y1+bevel, y2-bevel, c1); /* left */
  vline(bmp, x2, y1+bevel, y2-bevel, c2); /* right */

  line(bmp, x1, y1+bevel, x1+bevel, y1, c1); /* top-left */
  line(bmp, x1, y2-bevel, x1+bevel, y2, c2); /* bottom-left */

  line(bmp, x2-bevel, y1, x2, y1+bevel, c2); /* top-right */
  line(bmp, x2-bevel, y2, x2, y2-bevel, c2); /* bottom-right */
}

void rectdotted(BITMAP* bmp, int x1, int y1, int x2, int y2, int fg, int bg)
{
  simple_dotted_mode(bmp, fg, bg);
  rect(bmp, x1, y1, x2, y2, 0);
  solid_mode();
}

void rectgrid(BITMAP* bmp, int x1, int y1, int x2, int y2, int w, int h)
{
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

/**********************************************************************/
/* Specials */

void draw_emptyset_symbol(BITMAP* bmp, const Rect& rc, int color)
{
  Point center = rc.getCenter();
  int size = MIN(rc.w, rc.h) - 8;
  size = MID(4, size, 64);

  circle(bmp, center.x, center.y, size*4/10, color);
  line(bmp,
       center.x-size/2, center.y+size/2,
       center.x+size/2, center.y-size/2, color);
}

void draw_color(BITMAP* bmp, const Rect& rc, int imgtype, color_t color)
{
  int type = color_type(color);
  BITMAP* graph;

  if (type == COLOR_TYPE_MASK) {
    rectgrid(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, rc.w/4, rc.h/2);
    return;
  }
  else if (type == COLOR_TYPE_INDEX) {
    int index = color_get_index(color);

    if (index >= 0 && index < get_current_palette()->size()) {
      rectfill(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1,
	       get_color_for_allegro(bitmap_color_depth(bmp), color));
    }
    else {
      rectfill(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, makecol(0, 0, 0));
      line(bmp, rc.x+rc.w-2, rc.y+1, rc.x+1, rc.y+rc.h-2, makecol(255, 255, 255));
    }
    return;
  }

  switch (imgtype) {

    case IMAGE_INDEXED:
      rectfill(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1,
	       get_color_for_allegro(imgtype, color_index(get_color_for_image(imgtype, color))));
      break;

    case IMAGE_RGB:
      graph = create_bitmap_ex(32, rc.w, rc.h);
      if (!graph)
        return;

      {
        int rgb_bitmap_color = get_color_for_image(imgtype, color);
        color_t color2 = color_rgb(_rgba_getr(rgb_bitmap_color),
				   _rgba_getg(rgb_bitmap_color),
				   _rgba_getb(rgb_bitmap_color));
        rectfill(graph, 0, 0, rc.w-1, rc.h-1, get_color_for_allegro(32, color2));
      }

      blit(graph, bmp, 0, 0, rc.x, rc.y, rc.w, rc.h);

      destroy_bitmap(graph);
      break;

    case IMAGE_GRAYSCALE:
      graph = create_bitmap_ex(32, rc.w, rc.h);
      if (!graph)
        return;

      {
        int gray_bitmap_color = get_color_for_image(imgtype, color);
	color_t color2 = color_gray(_graya_getv(gray_bitmap_color));
        rectfill(graph, 0, 0, rc.w-1, rc.h-1, get_color_for_allegro(32, color2));
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
		       int imgtype, color_t color, bool hot, bool drag)
{
  SkinneableTheme* theme = (SkinneableTheme*)ji_get_theme();
  int scale = jguiscale();

  // Draw background (the color)
  draw_color(bmp,
	     Rect(rc.x+1*jguiscale(),
		  rc.y+1*jguiscale(),
		  rc.w-((outer_e) ? 2*jguiscale(): 1*jguiscale()),
		  rc.h-((outer_s) ? 2*jguiscale(): 1*jguiscale())), imgtype, color);

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

void draw_progress_bar(BITMAP* bmp,
		       int x1, int y1, int x2, int y2,
		       float progress)
{
  int w = x2 - x1 + 1;
  int u = (int)((float)(w-2)*progress);
  u = MID(0, u, w-2);

  rect(bmp, x1, y1, x2, y2, ji_color_foreground());

  if (u > 0)
    rectfill(bmp, x1+1, y1+1, x1+u, y2-1, ji_color_selected());

  if (1+u < w-2)
    rectfill(bmp, x1+u+1, y1+1, x2-1, y2-1, ji_color_background());
}

/************************************************************************/
/* Font related */

int character_length(FONT* font, int chr)
{
  return font->vtable->char_length(font, chr);
}

/* renders a character of the font centered in x and y */
void render_character(BITMAP* bmp, FONT* font, int chr, int x, int y, int fg, int bg)
{
  font->vtable->render_char(font, chr, fg, bg, bmp, x, y);
}
