/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007, 2008  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>

#include "jinete/jbase.h"

#include "core/app.h"
#include "core/core.h"
#include "modules/color.h"
#include "modules/gfx.h"
#include "modules/palette.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "widgets/colbar.h"

#endif

static struct {
  int type;
  int data;
} color_struct;

static void fill_color_struct(const char *color);
static int get_mask_for_bitmap(int depth);

int init_module_color(void)
{
  return 0;
}

void exit_module_color(void)
{
}

int color_type(const char *color)
{
  fill_color_struct(color);
  return color_struct.type;
}

char *color_mask(void)
{
  return jstrdup("mask");
}

char *color_rgb(int r, int g, int b, int a)
{
  char buf[256];
  usprintf(buf, "rgb{%d,%d,%d", r, g, b);
  if (a == 255)
    ustrcat(buf, "}");
  else
    usprintf(buf+ustrlen(buf), ",%d}", a);
  return jstrdup(buf);
}

char *color_gray(int g, int a)
{
  char buf[256];
  usprintf(buf, "gray{%d", g);
  if (a == 255)
    ustrcat(buf, "}");
  else
    usprintf(buf+ustrlen(buf), ",%d}", a);
  return jstrdup(buf);
}

char *color_index(int index)
{
  char buf[256];
  usprintf(buf, "index{%d}", index);
  return jstrdup(buf);
}

int color_get_black(int imgtype, const char *color)
{
  fill_color_struct(color);

  if (color_struct.type == COLOR_TYPE_MASK)
    return 0;
  else if (color_struct.type == COLOR_TYPE_RGB) {
    PRINTF("Getting `black' from a rgb color\n");
  }
  else if (color_struct.type == COLOR_TYPE_GRAY)
    return _graya_getk(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_INDEX) {
    PRINTF("Getting `black' from a index color\n");
  }

  return -1;
}

int color_get_red(int imgtype, const char *color)
{
  fill_color_struct(color);

  if (color_struct.type == COLOR_TYPE_MASK)
    return imgtype == IMAGE_INDEXED ? _rgb_scale_6[current_palette[0].r]: 0;
  else if (color_struct.type == COLOR_TYPE_RGB)
    return _rgba_getr(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_GRAY)
    return _graya_getk(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_INDEX)
    return _rgb_scale_6[current_palette[color_struct.data & 0xff].r];

  return -1;
}

int color_get_green(int imgtype, const char *color)
{
  fill_color_struct(color);

  if (color_struct.type == COLOR_TYPE_MASK)
    return imgtype == IMAGE_INDEXED ? _rgb_scale_6[current_palette[0].g]: 0;
  else if (color_struct.type == COLOR_TYPE_RGB)
    return _rgba_getg(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_GRAY)
    return _graya_getk(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_INDEX)
    return _rgb_scale_6[current_palette[color_struct.data & 0xff].g];

  return -1;
}

int color_get_blue(int imgtype, const char *color)
{
  fill_color_struct(color);

  if (color_struct.type == COLOR_TYPE_MASK)
    return imgtype == IMAGE_INDEXED ? _rgb_scale_6[current_palette[0].b]: 0;
  else if (color_struct.type == COLOR_TYPE_RGB)
    return _rgba_getb(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_GRAY)
    return _graya_getk(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_INDEX)
    return _rgb_scale_6[current_palette[color_struct.data & 0xff].b];

  return -1;
}

int color_get_index(int imgtype, const char *color)
{
  fill_color_struct(color);

  if (color_struct.type == COLOR_TYPE_MASK)
    return 0;
  else if (color_struct.type == COLOR_TYPE_RGB) {
    PRINTF("Getting `index' from a rgb color\n");
  }
  else if (color_struct.type == COLOR_TYPE_GRAY)
    return _graya_getk(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_INDEX)
    return color_struct.data & 0xff;

  return -1;
}

int color_get_alpha(int imgtype, const char *color)
{
  fill_color_struct(color);

  if (color_struct.type == COLOR_TYPE_MASK)
    return imgtype == IMAGE_INDEXED ? 255: 0;
  else if (color_struct.type == COLOR_TYPE_RGB)
    return _rgba_geta(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_GRAY)
    return _graya_geta(color_struct.data);
  else if (color_struct.type == COLOR_TYPE_INDEX)
    return 255;

  return -1;
}

/* set the alpha channel in the color */
void color_set_alpha(char **color, int alpha)
{
  fill_color_struct(*color);

  if (color_struct.type == COLOR_TYPE_RGB) {
    jfree(*color);
    *color = color_rgb(_rgba_getr(color_struct.data),
		       _rgba_getg(color_struct.data),
		       _rgba_getb(color_struct.data), alpha);
  }
  else if (color_struct.type == COLOR_TYPE_GRAY) {
    jfree(*color);
    *color = color_gray(_rgba_getg(color_struct.data), alpha);
  }
}

char *color_from_image(int imgtype, int c)
{
  char *color = NULL;

  switch (imgtype) {

    case IMAGE_RGB:
      if (_rgba_geta(c) > 0) {
	color = color_rgb(_rgba_getr(c),
			  _rgba_getg(c),
			  _rgba_getb(c),
			  _rgba_geta(c));
      }
      break;

    case IMAGE_GRAYSCALE:
      if (_graya_geta(c) > 0) {
	color = color_gray(_graya_getk(c),
			   _graya_geta(c));
      }
      break;

    case IMAGE_INDEXED:
      if (c > 0)
	color = color_index(c);
      break;
  }

  return (color) ? color: color_mask();
}

int blackandwhite(int r, int g, int b)
{
  return (r*30+g*59+b*11)/100 < 128 ?
    makecol(0, 0, 0): makecol(255, 255, 255);
}

int blackandwhite_neg(int r, int g, int b)
{
  return (r*30+g*59+b*11)/100 < 128 ?
    makecol(255, 255, 255): makecol(0, 0, 0);
}

int get_color_for_allegro(int depth, const char *color)
{
  int c;

  fill_color_struct(color);

  if (color_struct.type == COLOR_TYPE_MASK) {
    c = get_mask_for_bitmap(depth);
  }
  else if (color_struct.type == COLOR_TYPE_RGB) {
    c = makeacol_depth(depth,
		       _rgba_getr(color_struct.data),
		       _rgba_getg(color_struct.data),
		       _rgba_getb(color_struct.data),
		       _rgba_geta(color_struct.data));
  }
  else if (color_struct.type == COLOR_TYPE_GRAY) {
    c = _graya_getk(color_struct.data);

    if (depth != 8)
      c = makeacol_depth(depth, c, c, c, _graya_geta(color_struct.data));
  }
  else if (color_struct.type == COLOR_TYPE_INDEX) {
    c = color_struct.data & 0xff;

    if (depth != 8)
      c = makeacol_depth(depth,
			 _rgb_scale_6[current_palette[c].r],
			 _rgb_scale_6[current_palette[c].g],
			 _rgb_scale_6[current_palette[c].b], 255);
  }
  else
    c = -1;

  if (depth == 8 && c >= 0 && c < 256)
    c = _index_cmap[c];

  return c;
}

int get_color_for_image(int imgtype, const char *color)
{
  int c = -1;

  fill_color_struct(color);

  switch (imgtype) {

    case IMAGE_RGB:
      if (color_struct.type == COLOR_TYPE_MASK) {
        c = _rgba(0, 0, 0, 0);
      }
      else if (color_struct.type == COLOR_TYPE_RGB) {
	c = color_struct.data;
      }
      else if (color_struct.type == COLOR_TYPE_GRAY) {
        c = _graya_getk(color_struct.data);
        c = _rgba(c, c, c, _graya_geta(color_struct.data));
      }
      else if (color_struct.type == COLOR_TYPE_INDEX) {
        c = color_struct.data & 0xff;
        c = _rgba(_rgb_scale_6[current_palette[c].r],
		  _rgb_scale_6[current_palette[c].g],
		  _rgb_scale_6[current_palette[c].b], 255);
      }
      break;

    case IMAGE_GRAYSCALE:
      if (color_struct.type == COLOR_TYPE_MASK) {
        c = _graya(0, 0);
      }
      else if (color_struct.type == COLOR_TYPE_RGB) {
        int r = _rgba_getr(color_struct.data);
	int g = _rgba_getg(color_struct.data);
	int b = _rgba_getb(color_struct.data);

        rgb_to_hsv_int(&r, &g, &b);

        c = _graya(b, _rgba_geta(color_struct.data));
      }
      else if (color_struct.type == COLOR_TYPE_GRAY) {
        c = _graya(_graya_getk(color_struct.data),
		   _graya_geta(color_struct.data));
      }
      else if (color_struct.type == COLOR_TYPE_INDEX) {
        c = _graya(color_struct.data & 0xff, 255);
      }
      break;

    case IMAGE_INDEXED:
      if (color_struct.type == COLOR_TYPE_MASK) {
        c = 0;
      }
      else if (color_struct.type == COLOR_TYPE_RGB) {
	c = orig_rgb_map->data
	  [_rgba_getr(color_struct.data) >> 3]
	  [_rgba_getg(color_struct.data) >> 3]
	  [_rgba_getb(color_struct.data) >> 3];
      }
      else if (color_struct.type == COLOR_TYPE_GRAY) {
        c = _graya_getk(color_struct.data);
        c = orig_rgb_map->data[c>>3][c>>3][c>>3];
      }
      else if (color_struct.type == COLOR_TYPE_INDEX) {
        c = color_struct.data & 0xff;
      }
      break;
  }

  return c;
}

char *image_getpixel_color(Image *image, int x, int y)
{
  if ((x >= 0) && (y >= 0) && (x < image->w) && (y < image->h))
    return color_from_image(image->imgtype, image_getpixel(image, x, y));
  else
    return color_mask();
}

void color_to_formalstring(int imgtype, const char *color, char *buf,
			   int size, int long_format)
{
  fill_color_struct(color);

  /* long format */
  if (long_format) {
    if (color_struct.type == COLOR_TYPE_MASK) {
      uszprintf(buf, size, _("Mask"));
    }
    else if (color_struct.type == COLOR_TYPE_RGB) {
      if (imgtype == IMAGE_GRAYSCALE) {
        uszprintf(buf, size, "%s %d %s %d",
		  _("Gray"),
		  _graya_getk(get_color_for_image(imgtype, color)),
		  _("Alpha"),
		  _rgba_geta(color_struct.data));
      }
      else {
        uszprintf(buf, size, "%s %d %d %d",
		  _("RGB"),
		  _rgba_getr(color_struct.data),
		  _rgba_getg(color_struct.data),
		  _rgba_getb(color_struct.data));
  
        if (imgtype == IMAGE_INDEXED)
          uszprintf(buf+ustrlen(buf), size, " %s %d",
		    _("Index"), get_color_for_image(imgtype, color));
        else if (imgtype == IMAGE_RGB)
          uszprintf(buf+ustrlen(buf), size, " %s %d",
		    _("Alpha"), _rgba_geta(color_struct.data));
      }
    }
    else if (color_struct.type == COLOR_TYPE_GRAY) {
      uszprintf(buf, size, "%s %d",
		_("Gray"), _graya_getk(color_struct.data));

      if ((imgtype == IMAGE_RGB) ||
	  (imgtype == IMAGE_GRAYSCALE)) {
        uszprintf(buf+ustrlen(buf), size, " %s %d",
		  _("Alpha"), _graya_geta(color_struct.data));
      }
    }
    else if (color_struct.type == COLOR_TYPE_INDEX) {
      uszprintf(buf, size, "%s %d(RGB %d %d %d)",
		_("Index"),
		color_struct.data & 0xff,
		_rgb_scale_6[current_palette[color_struct.data & 0xff].r],
		_rgb_scale_6[current_palette[color_struct.data & 0xff].g],
		_rgb_scale_6[current_palette[color_struct.data & 0xff].b]);
    }
    else {
      uszprintf(buf, size, _("Unknown"));
    }
  }
  /* short format */
  else {
    if (color_struct.type == COLOR_TYPE_MASK) {
      uszprintf(buf, size, "MASK");
    }
    else if (color_struct.type == COLOR_TYPE_RGB) {
      if (imgtype == IMAGE_GRAYSCALE) {
        uszprintf(buf, size, "KA %02x(%d)",
		  _graya_getk(get_color_for_image(imgtype, color)),
		  _graya_getk(get_color_for_image(imgtype, color)));
      }
      else {
        uszprintf(buf, size, "RGB %02x%02x%02x",
                  _rgba_getr(color_struct.data),
		  _rgba_getg(color_struct.data),
		  _rgba_getb(color_struct.data));

        if (imgtype == IMAGE_INDEXED)
          uszprintf(buf+ustrlen(buf), size, "(%d)",
		    get_color_for_image(imgtype, color));
      }
    }
    else if (color_struct.type == COLOR_TYPE_GRAY) {
      uszprintf(buf, size, "I %02x (%d)",
                _graya_getk(color_struct.data),
                _graya_getk(color_struct.data));
    }
    else if (color_struct.type == COLOR_TYPE_INDEX) {
      uszprintf(buf, size, "I %02x (%d)",
                color_struct.data & 0xff,
                color_struct.data & 0xff);
    }
    else {
      uszprintf(buf, size, "?");
    }
  }
}

void draw_color(BITMAP *bmp, int x1, int y1, int x2, int y2,
		int imgtype, const char *color)
{
  int w = x2 - x1 + 1;
  int h = y2 - y1 + 1;
  BITMAP *graph;
  int grid;

  grid = MIN(w, h) / 2;
  grid += MIN(w, h) - grid*2;

  fill_color_struct(color);

  if (color_struct.type == COLOR_TYPE_INDEX) {
    rectfill(bmp, x1, y1, x2, y2,
	     /* get_color_for_allegro(bitmap_color_depth(bmp), color)); */
	     palette_color[_index_cmap[color_struct.data & 0xff]]);
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

      rectgrid(graph, 0, 0, w-1, h-1, grid, grid);

      drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
      set_trans_blender(0, 0, 0, color_get_alpha(imgtype, color));
      {
        int rgb_bitmap_color = get_color_for_image(imgtype, color);
        char *color2 = color_rgb(_rgba_getr(rgb_bitmap_color),
				 _rgba_getg(rgb_bitmap_color),
				 _rgba_getb(rgb_bitmap_color),
				 _rgba_geta(rgb_bitmap_color));
        rectfill(graph, 0, 0, w-1, h-1, get_color_for_allegro(32, color2));
        jfree(color2);
      }
      drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);

      use_current_sprite_rgb_map();
      blit(graph, bmp, 0, 0, x1, y1, w, h);
      restore_rgb_map();

      destroy_bitmap(graph);
      break;

    case IMAGE_GRAYSCALE:
      graph = create_bitmap_ex(32, w, h);
      if (!graph)
        return;

      rectgrid(graph, 0, 0, w-1, h-1, grid, grid);

      drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
      set_trans_blender(0, 0, 0, color_get_alpha(imgtype, color));
      {
        int gray_bitmap_color = get_color_for_image(imgtype, color);
        char *color2 = color_gray(_graya_getk(gray_bitmap_color),
				  _graya_geta(gray_bitmap_color));
        rectfill(graph, 0, 0, w-1, h-1, get_color_for_allegro(32, color2));
        jfree(color2);
      }
      drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);

      use_current_sprite_rgb_map();
      blit(graph, bmp, 0, 0, x1, y1, w, h);
      restore_rgb_map();

      destroy_bitmap(graph);
      break;
  }
}

static void fill_color_struct(const char *color)
{
  char *buf, *tok;

  color_struct.type = COLOR_TYPE_MASK;
  color_struct.data = 0;

  buf = color ? jstrdup(color): NULL;
  if (!buf)
    return;

  if (ustrcmp (buf, "mask") != 0) {
    if (ustrncmp (buf, "rgb{", 4) == 0) {
      int c=0, table[4] = { 0, 0, 0, 255 };

      for (tok=ustrtok(buf+4, ","); tok;
	   tok=ustrtok(NULL, ",")) {
	if (c < 4)
	  table[c++] = ustrtol(tok, NULL, 10);
      }

      color_struct.type = COLOR_TYPE_RGB;
      color_struct.data = _rgba(table[0], table[1], table[2], table[3]);
    } 
    else if (ustrncmp (buf, "gray{", 5) == 0) {
      int c=0, table[2] = { 0, 255 };

      for (tok=ustrtok(buf+5, ","); tok;
	   tok=ustrtok(NULL, ",")) {
	if (c < 2)
	  table[c++] = ustrtol(tok, NULL, 10);
      }

      color_struct.type = COLOR_TYPE_GRAY;
      color_struct.data = _graya(table[0], table[1]);
    }
    else if (ustrncmp (buf, "index{", 6) == 0) {
      color_struct.type = COLOR_TYPE_INDEX;
      color_struct.data = ustrtol(buf+6, NULL, 10);
    }
  }

  jfree(buf);
}

/* returns the same values of bitmap_mask_color() (this function *must*
   returns the same values) */
static int get_mask_for_bitmap(int depth)
{
  switch (depth) {
    case  8: return MASK_COLOR_8;  break;
    case 15: return MASK_COLOR_15; break;
    case 16: return MASK_COLOR_16; break;
    case 24: return MASK_COLOR_24; break;
    case 32: return MASK_COLOR_32; break;
    default:
      return -1;
  }
}
