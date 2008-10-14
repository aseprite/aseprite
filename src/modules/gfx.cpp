/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"

#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/dirs.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/tools.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "widgets/editor.h"

/* static BITMAP *icons_pcx; */
static BITMAP *gfx_bmps[GFX_BITMAP_COUNT];

#include "modules/gfxdata.cpp"

static void convert_data_to_bitmap(DATA *data, BITMAP **bmp)
{
  int scale = guiscale();
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

static void gen_gfx(void *data)
{
  int c;

  for (c=0; c<GFX_BITMAP_COUNT; c++) {
    if (gfx_bmps[c])
      destroy_bitmap(gfx_bmps[c]);

    gfx_bmps[c] = NULL;
  }
}

int init_module_graphics()
{
  int c;
  
  for (c=0; c<GFX_BITMAP_COUNT; c++)
    gfx_bmps[c] = NULL;

  app_add_hook(APP_PALETTE_CHANGE, gen_gfx, NULL);
  return 0;
}

void exit_module_graphics()
{
  int c;

  for (c=0; c<GFX_BITMAP_COUNT; c++)
    if (gfx_bmps[c]) {
      destroy_bitmap(gfx_bmps[c]);
      gfx_bmps[c] = NULL;
    }
}

BITMAP *get_gfx(int id)
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

  static BITMAP *pattern = NULL;
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

void simple_dotted_mode(BITMAP *bmp, int fg, int bg)
{
  static BITMAP *pattern = NULL;

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
/* Set/Restore sub-clip regions */

struct CLIP_DATA
{
  BITMAP* bmp;
  int cl, ct, cr, cb;
};

void* subclip(BITMAP* bmp, int x1, int y1, int x2, int y2)
{
  int cl, ct, cr, cb;
  CLIP_DATA* data;

  cl = bmp->cl;
  ct = bmp->ct;
  cr = bmp->cr;
  cb = bmp->cb;

  if ((x2 < cl) || (x1 >= cr) || (y2 < ct) || (y1 >= cb))
    return NULL;

  x1 = MID(cl, x1, cr-1);
  y1 = MID(ct, y1, cb-1);
  x2 = MID(x1, x2, cr-1);
  y2 = MID(y1, y2, cb-1);

  set_clip(bmp, x1, y1, x2, y2);

  data = new CLIP_DATA;
  data->bmp = bmp;
  data->cl = cl;
  data->ct = ct;
  data->cr = cr;
  data->cb = cb;

  return data;
}

void backclip(void* _data)
{
  CLIP_DATA* data = reinterpret_cast<CLIP_DATA*>(_data);
  set_clip(data->bmp, data->cl, data->ct, data->cr, data->cb);
  delete data;
}

/**********************************************************************/
/* Rectangle Tracker (Save/Restore rectangles from/to the screen) */

struct RectTracker
{
  BITMAP *bmp;
  int x1, y1, x2, y2;
  int npixel;
  int *pixel;
};

static void do_rect(BITMAP* bmp, int x1, int y1, int x2, int y2, int c,
		    void (*proc)(BITMAP* bmp, int x, int y, int c))
{
  int x, y, u1, u2, v1, v2;

  if ((x2 < bmp->cl) || (x1 >= bmp->cr) ||
      (y2 < bmp->ct) || (y1 >= bmp->cb))
    return;

  u1 = MID(bmp->cl, x1, bmp->cr-1);
  u2 = MID(x1, x2, bmp->cr-1);

  if ((y1 >= bmp->ct) && (y1 < bmp->cb)) {
    for (x=u1; x<=u2; x++)
      (*proc) (bmp, x, y1, c);
  }

  if ((y2 >= bmp->ct) && (y2 < bmp->cb)) {
    for (x=u1; x<=u2; x++)
      (*proc) (bmp, x, y2, c);
  }

  v1 = MID(bmp->ct, y1+1, bmp->cb-1);
  v2 = MID(v1, y2-1, bmp->cb-1);

  if ((x1 >= bmp->cl) && (x1 < bmp->cr)) {
    for (y=v1; y<=v2; y++)
      (*proc)(bmp, x1, y, c);
  }

  if ((x2 >= bmp->cl) && (x2 < bmp->cr)) {
    for (y=v1; y<=v2; y++)
      (*proc)(bmp, x2, y, c);
  }
}

static void count_rect(BITMAP* bmp, int x, int y, int c)
{
  RectTracker* data = (RectTracker*)c;
  data->npixel++;
}

static void save_rect(BITMAP* bmp, int x, int y, int c)
{
  RectTracker* data = (RectTracker*)c;
  data->pixel[data->npixel++] = getpixel(bmp, x, y);
}

static void restore_rect(BITMAP* bmp, int x, int y, int c)
{
  RectTracker* data = (RectTracker*)c;
  putpixel(bmp, x, y, data->pixel[data->npixel++]);
}

RectTracker* rect_tracker_new(BITMAP* bmp, int x1, int y1, int x2, int y2)
{
  RectTracker *data;
  int x, y;

  jmouse_hide();

  if (x1 > x2) { x = x1; x1 = x2; x2 = x; }
  if (y1 > y2) { y = y1; y1 = y2; y2 = y; }

  data = new RectTracker;

  data->bmp = bmp;
  data->x1 = x1;
  data->y1 = y1;
  data->x2 = x2;
  data->y2 = y2;

  data->npixel = 0;
  do_rect(bmp, x1, y1, x2, y2, (int)data, count_rect);

  if (data->npixel > 0)
    data->pixel = new int[data->npixel];
  else
    data->pixel = NULL;

  data->npixel = 0;
  do_rect(bmp, x1, y1, x2, y2, (int)data, save_rect);

  jmouse_show();

  return data;
}

void rect_tracker_free(RectTracker* data)
{
  jmouse_hide();

  data->npixel = 0;
  do_rect(data->bmp, data->x1, data->y1, data->x2, data->y2,
	  (int)data, restore_rect);

  delete[] data->pixel;
  delete data;

  jmouse_show();
}

/**********************************************************************/
/* Rectangles */

void bevel_box(BITMAP *bmp, int x1, int y1, int x2, int y2, int c1, int c2, int bevel)
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

void rectdotted(BITMAP *bmp, int x1, int y1, int x2, int y2, int fg, int bg)
{
  simple_dotted_mode(bmp, fg, bg);
  rect(bmp, x1, y1, x2, y2, 0);
  solid_mode();
}

void rectgrid(BITMAP *bmp, int x1, int y1, int x2, int y2, int w, int h)
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

void draw_emptyset_symbol(JRect rc, int color)
{
  int cx, cy, x1, y1, x2, y2, size;

  size = MIN(jrect_w(rc), jrect_h(rc)) - 8;
  size = MID(4, size, 64);

  cx = (rc->x1+rc->x2)/2;
  cy = (rc->y1+rc->y2)/2;
  x1 = cx-size/2;
  y1 = cy-size/2;
  x2 = x1+size-1;
  y2 = y1+size-1;

  circle(ji_screen, cx, cy, size*4/10, color);
  line(ji_screen, x1, y2, x2, y1, color);
}

void draw_color(BITMAP *bmp, int x1, int y1, int x2, int y2,
		int imgtype, color_t color)
{
  int type = color_type(color);
  int data;
  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;
  BITMAP *graph;

  if (type == COLOR_TYPE_INDEX) {
    data = color_get_index(imgtype, color);
    rectfill(bmp, x1, y1, x2, y2,
	     /* get_color_for_allegro(bitmap_color_depth(bmp), color)); */
	     palette_color[_index_cmap[data]]);
    return;
  }

  switch (imgtype) {

    case IMAGE_INDEXED:
      rectfill(bmp, x1, y1, x2, y2,
	       palette_color[_index_cmap[get_color_for_image(imgtype, color)]]);
      break;

    case IMAGE_RGB:
      graph = create_bitmap_ex(32, w, h);
      if (!graph)
        return;

      {
        int rgb_bitmap_color = get_color_for_image(imgtype, color);
        color_t color2 = color_rgb(_rgba_getr(rgb_bitmap_color),
				   _rgba_getg(rgb_bitmap_color),
				   _rgba_getb(rgb_bitmap_color));
        rectfill(graph, 0, 0, w-1, h-1, get_color_for_allegro(32, color2));
      }

      use_current_sprite_rgb_map();
      blit(graph, bmp, 0, 0, x1, y1, w, h);
      restore_rgb_map();

      destroy_bitmap(graph);
      break;

    case IMAGE_GRAYSCALE:
      graph = create_bitmap_ex(32, w, h);
      if (!graph)
        return;

      {
        int gray_bitmap_color = get_color_for_image(imgtype, color);
	color_t color2 = color_gray(_graya_getv(gray_bitmap_color));
        rectfill(graph, 0, 0, w-1, h-1, get_color_for_allegro(32, color2));
      }

      use_current_sprite_rgb_map();
      blit(graph, bmp, 0, 0, x1, y1, w, h);
      restore_rgb_map();

      destroy_bitmap(graph);
      break;
  }
}

void draw_color_button(BITMAP *bmp,
		       int x1, int y1, int x2, int y2,
		       int b0, int b1, int b2, int b3,
		       int imgtype, color_t color,
		       bool hot, bool drag)
{
  int face = ji_color_face();
  int fore = ji_color_foreground();

  draw_color(bmp, x1, y1, x2, y2, imgtype, color);

  hline(bmp, x1, y1, x2, fore);
  if (b2 && b3)
    hline(bmp, x1, y2, x2, fore);
  vline(bmp, x1, y1, y2, fore);
  vline(bmp, x2, y1, y2, fore);

  if (!hot) {
    int r = color_get_red(imgtype, color);
    int g = color_get_green(imgtype, color);
    int b = color_get_blue(imgtype, color);
    int c = makecol(MIN(255, r+64),
		    MIN(255, g+64),
		    MIN(255, b+64));
    rect(bmp, x1+1, y1+1, x2-1, y2-((b2 && b3)?1:0), c);
  }
  else {
    rect(bmp, x1+1, y1+1, x2-1, y2-((b2 && b3)?1:0), fore);
    bevel_box(bmp,
	      x1+1, y1+1, x2-1, y2-((b2 && b3)?1:0),
	      ji_color_facelight(), ji_color_faceshadow(), 1);
  }

  if (b0) {
    hline(bmp, x1, y1, x1+1, face);
    putpixel(bmp, x1, y1+1, face);
    putpixel(bmp, x1+1, y1+1, fore);
  }

  if (b1) {
    hline(bmp, x2-1, y1, x2, face);
    putpixel(bmp, x2, y1+1, face);
    putpixel(bmp, x2-1, y1+1, fore);
  }

  if (b2) {
    putpixel(bmp, x1, y2-1, face);
    hline(bmp, x1, y2, x1+1, face);
    putpixel(bmp, x1+1, y2-1, fore);
  }

  if (b3) {
    putpixel(bmp, x2, y2-1, face);
    hline(bmp, x2-1, y2, x2, face);
    putpixel(bmp, x2-1, y2-1, fore);
  }

  if (drag) {
    rect(bmp, x1+2, y1+2, x2-2, y2-2,
	 blackandwhite_neg(color_get_red(imgtype, color),
			   color_get_green(imgtype, color),
			   color_get_blue(imgtype, color)));
  }
}

/************************************************************************/
/* Font related */

int character_length(FONT *font, int chr)
{
  return font->vtable->char_length(font, chr);
}

/* renders a character of the font centered in x and y */
void render_character(BITMAP *bmp, FONT *font, int chr, int x, int y, int fg, int bg)
{
  font->vtable->render_char(font, chr, fg, bg, bmp, x, y);
}
