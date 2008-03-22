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

#include <assert.h>
#include <allegro.h>

#include "jinete/jbase.h"

#include "core/app.h"
#include "core/color.h"
#include "core/core.h"
#include "modules/gfx.h"
#include "modules/palettes.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "widgets/colbar.h"

#define _hsva_geth		_rgba_getr
#define _hsva_gets		_rgba_getg
#define _hsva_getv		_rgba_getb
#define _hsva_geta		_rgba_geta
#define _hsva			_rgba

#define GET_COLOR_TYPE(color)	((ase_uint32)((color).coltype))
#define GET_COLOR_DATA(color)	((ase_uint32)((color).imgcolor))

#define GET_COLOR_RGB(color)	(GET_COLOR_DATA(color) & 0xffffffff)
#define GET_COLOR_HSV(color)	(GET_COLOR_DATA(color) & 0xffffffff)
#define GET_COLOR_GRAY(color)	(GET_COLOR_DATA(color) & 0xffff)
#define GET_COLOR_INDEX(color)	(GET_COLOR_DATA(color) & 0xff)

static int get_mask_for_bitmap(int depth);

char *color_to_string(color_t color, char *buf, int size)
{
  int data;

  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      uszprintf(buf, size, "mask");
      break;

    case COLOR_TYPE_RGB:
      data = GET_COLOR_RGB(color);
      uszprintf(buf, size, "rgb{%d,%d,%d,%d}",
		_rgba_getr(data),
		_rgba_getg(data),
		_rgba_getb(data),
		_rgba_geta(data));
      break;

    case COLOR_TYPE_HSV:
      data = GET_COLOR_HSV(color);
      uszprintf(buf, size, "hsv{%d,%d,%d,%d}",
		_hsva_geth(data),
		_hsva_gets(data),
		_hsva_getv(data),
		_hsva_geta(data));
      break;

    case COLOR_TYPE_GRAY:
      data = GET_COLOR_GRAY(color);
      uszprintf(buf, size, "gray{%d,%d}",
		_graya_getv(data),
		_graya_geta(data));
      break;

    case COLOR_TYPE_INDEX:
      data = GET_COLOR_INDEX(color);
      uszprintf(buf, size, "index{%d}",
		data);
      break;

  }
  return buf;
}

color_t string_to_color(const char *_str)
{
  char *str = jstrdup(_str);
  color_t color = color_mask();
  char *tok;

  if (ustrcmp(str, "mask") != 0) {
    if (ustrncmp(str, "rgb{", 4) == 0) {
      int c=0, table[4] = { 0, 0, 0, 255 };

      for (tok=ustrtok(str+4, ","); tok;
	   tok=ustrtok(NULL, ",")) {
	if (c < 4)
	  table[c++] = ustrtol(tok, NULL, 10);
      }

      color = color_rgb(table[0], table[1], table[2], table[3]);
    } 
    else if (ustrncmp(str, "hsv{", 4) == 0) {
      int c=0, table[4] = { 0, 0, 0, 255 };

      for (tok=ustrtok(str+4, ","); tok;
	   tok=ustrtok(NULL, ",")) {
	if (c < 4)
	  table[c++] = ustrtol(tok, NULL, 10);
      }

      color = color_hsv(table[0], table[1], table[2], table[3]);
    } 
    else if (ustrncmp(str, "gray{", 5) == 0) {
      int c=0, table[2] = { 0, 255 };

      for (tok=ustrtok(str+5, ","); tok;
	   tok=ustrtok(NULL, ",")) {
	if (c < 2)
	  table[c++] = ustrtol(tok, NULL, 10);
      }

      color = color_gray(table[0], table[1]);
    }
    else if (ustrncmp(str, "index{", 6) == 0) {
      color = color_index(ustrtol(str+6, NULL, 10));
    }
  }

  jfree(str);
  return color;
}

int color_type(color_t color)
{
  return GET_COLOR_TYPE(color);
}

bool color_equals(color_t c1, color_t c2)
{
  return
    GET_COLOR_TYPE(c1) == GET_COLOR_TYPE(c2) &&
    GET_COLOR_DATA(c1) == GET_COLOR_DATA(c2);
}

color_t color_mask(void)
{
  color_t c = { COLOR_TYPE_MASK, 0 };
  return c;
}

color_t color_rgb(int r, int g, int b, int a)
{
  color_t c = { COLOR_TYPE_RGB,
		_rgba(r & 0xff,
		      g & 0xff,
		      b & 0xff,
		      a & 0xff) };
  return c;
}

color_t color_hsv(int h, int s, int v, int a)
{
  color_t c = { COLOR_TYPE_HSV,
		_hsva(h & 0xff,
		      s & 0xff,
		      v & 0xff,
		      a & 0xff) };
  return c;
}

color_t color_gray(int g, int a)
{
  color_t c = { COLOR_TYPE_GRAY,
		_graya(g & 0xff,
		       a & 0xff) };
  return c;
}

color_t color_index(int index)
{
  color_t c = { COLOR_TYPE_INDEX,
		index & 0xff };
  return c;
}

int color_get_red(int imgtype, color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      if (imgtype == IMAGE_INDEXED)
	return _rgba_getr(palette_get_entry(get_current_palette(), 0));
      else
	return 0;

    case COLOR_TYPE_RGB:
      return _rgba_getr(GET_COLOR_RGB(color));

    case COLOR_TYPE_HSV: {
      int c = GET_COLOR_HSV(color);
      int h = _hsva_geth(c);
      int s = _hsva_gets(c);
      int v = _hsva_getv(c);
      hsv_to_rgb_int(&h, &s, &v);
      return h;
    }
      
    case COLOR_TYPE_GRAY:
      return _graya_getv(GET_COLOR_GRAY(color));

    case COLOR_TYPE_INDEX:
      return _rgba_getr(palette_get_entry(get_current_palette(),
					  GET_COLOR_INDEX(color)));

  }

  assert(FALSE);
  return -1;
}

int color_get_green(int imgtype, color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      if (imgtype == IMAGE_INDEXED)
	return _rgba_getg(palette_get_entry(get_current_palette(), 0));
      else
	return 0;

    case COLOR_TYPE_RGB:
      return _rgba_getg(GET_COLOR_RGB(color));

    case COLOR_TYPE_HSV: {
      int c = GET_COLOR_HSV(color);
      int h = _hsva_geth(c);
      int s = _hsva_gets(c);
      int v = _hsva_getv(c);
      hsv_to_rgb_int(&h, &s, &v);
      return s;
    }
      
    case COLOR_TYPE_GRAY:
      return _graya_getv(GET_COLOR_GRAY(color));

    case COLOR_TYPE_INDEX:
      return _rgba_getg(palette_get_entry(get_current_palette(),
					  GET_COLOR_INDEX(color)));

  }

  assert(FALSE);
  return -1;
}

int color_get_blue(int imgtype, color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      if (imgtype == IMAGE_INDEXED)
	return _rgba_getb(palette_get_entry(get_current_palette(), 0));
      else
	return 0;

    case COLOR_TYPE_RGB:
      return _rgba_getb(GET_COLOR_RGB(color));

    case COLOR_TYPE_HSV: {
      int c = GET_COLOR_HSV(color);
      int h = _hsva_geth(c);
      int s = _hsva_gets(c);
      int v = _hsva_getv(c);
      hsv_to_rgb_int(&h, &s, &v);
      return v;
    }
      
    case COLOR_TYPE_GRAY:
      return _graya_getv(GET_COLOR_GRAY(color));

    case COLOR_TYPE_INDEX:
      return _rgba_getb(palette_get_entry(get_current_palette(),
					  GET_COLOR_INDEX(color)));

  }

  assert(FALSE);
  return -1;
}

int color_get_hue(int imgtype, color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB: {
      int c = GET_COLOR_RGB(color);
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return r;
    }

    case COLOR_TYPE_HSV:
      return _hsva_geth(GET_COLOR_HSV(color));

    case COLOR_TYPE_GRAY:
      return 0;

    case COLOR_TYPE_INDEX: {
      ase_uint32 c = palette_get_entry(get_current_palette(),
				       GET_COLOR_INDEX(color));
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return r;
    }

  }

  assert(FALSE);
  return -1;
}

int color_get_saturation(int imgtype, color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB: {
      int c = GET_COLOR_RGB(color);
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return g;
    }

    case COLOR_TYPE_HSV:
      return _hsva_gets(GET_COLOR_HSV(color));

    case COLOR_TYPE_GRAY:
      return 0;

    case COLOR_TYPE_INDEX: {
      ase_uint32 c = palette_get_entry(get_current_palette(),
				       GET_COLOR_INDEX(color));
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return g;
    }

  }

  assert(FALSE);
  return -1;
}

int color_get_value(int imgtype, color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB: {
      int c = GET_COLOR_RGB(color);
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return b;
    }

    case COLOR_TYPE_HSV:
      return _hsva_getv(GET_COLOR_HSV(color));
      
    case COLOR_TYPE_GRAY:
      return _graya_getv(GET_COLOR_GRAY(color));

    case COLOR_TYPE_INDEX: {
      ase_uint32 c = palette_get_entry(get_current_palette(),
				       GET_COLOR_INDEX(color));
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return b;
    }

  }

  assert(FALSE);
  return -1;
}

int color_get_index(int imgtype, color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB:
      PRINTF("Getting `index' from a RGB color\n"); /* TODO */
      assert(FALSE);
      break;

    case COLOR_TYPE_HSV:
      PRINTF("Getting `index' from a HSV color\n"); /* TODO */
      assert(FALSE);
      break;

    case COLOR_TYPE_GRAY:
      return _graya_getv(GET_COLOR_GRAY(color));

    case COLOR_TYPE_INDEX:
      return GET_COLOR_INDEX(color);

  }

  assert(FALSE);
  return -1;
}

int color_get_alpha(int imgtype, color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return imgtype == IMAGE_INDEXED ? 255: 0;

    case COLOR_TYPE_RGB:
      return _rgba_geta(GET_COLOR_RGB(color));

    case COLOR_TYPE_HSV:
      return _hsva_geta(GET_COLOR_HSV(color));
      
    case COLOR_TYPE_GRAY:
      return _graya_geta(GET_COLOR_RGB(color));

    case COLOR_TYPE_INDEX:
      return 255;

  }

  assert(FALSE);
  return -1;
}

/* set the alpha channel in the color */
void color_set_alpha(color_t *color, int alpha)
{
  int data;

  switch (GET_COLOR_TYPE(*color)) {

    case COLOR_TYPE_MASK:
      assert(FALSE);
      break;

    case COLOR_TYPE_RGB:
      data = GET_COLOR_RGB(*color);
      *color = color_rgb(_rgba_getr(data),
			 _rgba_getg(data),
			 _rgba_getb(data), alpha);
      break;

    case COLOR_TYPE_HSV:
      data = GET_COLOR_HSV(*color);
      *color = color_hsv(_hsva_geth(data),
			 _hsva_gets(data),
			 _hsva_getv(data), alpha);
      break;
      
    case COLOR_TYPE_GRAY:
      data = GET_COLOR_GRAY(*color);
      *color = color_gray(_rgba_getg(data), alpha);
      break;

    case COLOR_TYPE_INDEX:
      assert(FALSE);
      break;

  }
}

color_t color_from_image(int imgtype, int c)
{
  color_t color = color_mask();

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
	color = color_gray(_graya_getv(c),
			   _graya_geta(c));
      }
      break;

    case IMAGE_INDEXED:
      if (c > 0)
	color = color_index(c);
      break;
  }

  return color;
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

int get_color_for_allegro(int depth, color_t color)
{
  int data;
  int c = -1;
  
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      c = get_mask_for_bitmap(depth);
      break;

    case COLOR_TYPE_RGB:
      data = GET_COLOR_RGB(color);
      c = makeacol_depth(depth,
			 _rgba_getr(data),
			 _rgba_getg(data),
			 _rgba_getb(data),
			 _rgba_geta(data));
      break;

    case COLOR_TYPE_HSV: {
      int h, s, v;

      data = GET_COLOR_HSV(color);
      h = _hsva_geth(data);
      s = _hsva_gets(data);
      v = _hsva_getv(data);
      hsv_to_rgb_int(&h, &s, &v);

      c = makeacol_depth(depth, h, s, v, _hsva_geta(data));
      break;
    }
      
    case COLOR_TYPE_GRAY:
      data = GET_COLOR_GRAY(color);
      c = _graya_getv(data);
      if (depth != 8)
	c = makeacol_depth(depth, c, c, c, _graya_geta(data));
      break;

    case COLOR_TYPE_INDEX:
      c = GET_COLOR_INDEX(color);
      if (depth != 8) {
	ase_uint32 _c = palette_get_entry(get_current_palette(), c);
	c = makeacol_depth(depth,
			   _rgba_getr(_c),
			   _rgba_getg(_c),
			   _rgba_getb(_c), 255);
      }
      break;

  }

  if (depth == 8 && c >= 0 && c < 256)
    c = _index_cmap[c];

  return c;
}

int get_color_for_image(int imgtype, color_t color)
{
  int c = -1;
  int data;

  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      switch (imgtype) {
	case IMAGE_RGB:
	  c = _rgba(0, 0, 0, 0);
	  break;
	case IMAGE_GRAYSCALE:
	  c = _graya(0, 0);
	  break;
	case IMAGE_INDEXED:
	  c = 0;
	  break;
      }
      break;

    case COLOR_TYPE_RGB:
      data = GET_COLOR_RGB(color);
      switch (imgtype) {
	case IMAGE_RGB:
	  c = data;
	  break;
	case IMAGE_GRAYSCALE: {
	  int r = _rgba_getr(data);
	  int g = _rgba_getg(data);
	  int b = _rgba_getb(data);

	  rgb_to_hsv_int(&r, &g, &b);

	  c = _graya(b, _rgba_geta(data));
	  break;
	}
	case IMAGE_INDEXED:
	  c = orig_rgb_map->data
	    [_rgba_getr(data) >> 3]
	    [_rgba_getg(data) >> 3]
	    [_rgba_getb(data) >> 3];
	  break;
      }
      break;


    case COLOR_TYPE_HSV: {
      int h, s, v;

      data = GET_COLOR_HSV(color);
      
      switch (imgtype) {
	case IMAGE_RGB:
	  h = _hsva_geth(data);
	  s = _hsva_gets(data);
	  v = _hsva_getv(data);
	  hsv_to_rgb_int(&h, &s, &v);

	  c = _rgba(h, s, v, _hsva_geta(data));
	  break;
	case IMAGE_GRAYSCALE: {
	  c = _graya(_hsva_getv(data),
		     _hsva_geta(data));
	  break;
	}
	case IMAGE_INDEXED:
	  h = _hsva_geth(data);
	  s = _hsva_gets(data);
	  v = _hsva_getv(data);
	  hsv_to_rgb_int(&h, &s, &v);

	  c = orig_rgb_map->data[h >> 3][s >> 3][v >> 3];
	  break;
      }
      break;
    }
      
    case COLOR_TYPE_GRAY:
      data = GET_COLOR_GRAY(color);
      switch (imgtype) {
	case IMAGE_RGB:
	  c = _graya_getv(data);
	  c = _rgba(c, c, c, _graya_geta(data));
	  break;
	case IMAGE_GRAYSCALE:
	  c = data;
	  break;
	case IMAGE_INDEXED:
	  c = _graya_getv(data);
	  c = orig_rgb_map->data[c>>3][c>>3][c>>3];
	  break;
      }
      break;

    case COLOR_TYPE_INDEX:
      data = GET_COLOR_INDEX(color);
      switch (imgtype) {
	case IMAGE_RGB: {
	  ase_uint32 _c = palette_get_entry(get_current_palette(), data);
	  c = _rgba(_rgba_getr(_c),
		    _rgba_getg(_c),
		    _rgba_getb(_c), 255);
	  break;
	}
	case IMAGE_GRAYSCALE:
	  c = _graya(data & 0xff, 255);
	  break;
	case IMAGE_INDEXED:
	  c = data & 0xff;
	  break;
      }
      break;

  }

  return c;
}

color_t image_getpixel_color(Image *image, int x, int y)
{
  if ((x >= 0) && (y >= 0) && (x < image->w) && (y < image->h))
    return color_from_image(image->imgtype, image_getpixel(image, x, y));
  else
    return color_mask();
}

void color_to_formalstring(int imgtype, color_t color,
			   char *buf, int size, bool long_format)
{
  int data;

  /* long format */
  if (long_format) {
    switch (GET_COLOR_TYPE(color)) {

      case COLOR_TYPE_MASK:
	uszprintf(buf, size, _("Mask"));
	break;

      case COLOR_TYPE_RGB:
	data = GET_COLOR_RGB(color);
	if (imgtype == IMAGE_GRAYSCALE) {
	  uszprintf(buf, size, "%s %d %s %d",
		    _("Gray"),
		    _graya_getv(get_color_for_image(imgtype, color)),
		    _("Alpha"),
		    _rgba_geta(data));
	}
	else {
	  uszprintf(buf, size, "%s %d %d %d",
		    _("RGB"),
		    _rgba_getr(data),
		    _rgba_getg(data),
		    _rgba_getb(data));
  
	  if (imgtype == IMAGE_INDEXED)
	    uszprintf(buf+ustrlen(buf), size, " %s %d",
		      _("Index"), get_color_for_image(imgtype, color));
	  else if (imgtype == IMAGE_RGB)
	    uszprintf(buf+ustrlen(buf), size, " %s %d",
		      _("Alpha"), _rgba_geta(data));
	}
	break;

      case COLOR_TYPE_HSV:
	data = GET_COLOR_HSV(color);
	if (imgtype == IMAGE_GRAYSCALE) {
	  uszprintf(buf, size, "%s %d %s %d",
		    _("Gray"), _hsva_getv(data),
		    _("Alpha"), _hsva_geta(data));
	}
	else {
	  uszprintf(buf, size, "%s %d %d %d",
		    _("HSV"),
		    _hsva_geth(data),
		    _hsva_gets(data),
		    _hsva_getv(data));
  
	  if (imgtype == IMAGE_INDEXED)
	    uszprintf(buf+ustrlen(buf), size, " %s %d",
		      _("Index"), get_color_for_image(imgtype, color));
	  else if (imgtype == IMAGE_RGB)
	    uszprintf(buf+ustrlen(buf), size, " %s %d",
		      _("Alpha"), _hsva_geta(data));
	}
	break;
	
      case COLOR_TYPE_GRAY:
	data = GET_COLOR_GRAY(color);
	uszprintf(buf, size, "%s %d",
		  _("Gray"), _graya_getv(data));

	if ((imgtype == IMAGE_RGB) ||
	    (imgtype == IMAGE_GRAYSCALE)) {
	  uszprintf(buf+ustrlen(buf), size, " %s %d",
		    _("Alpha"), _graya_geta(data));
	}
	break;

      case COLOR_TYPE_INDEX: {
	ase_uint32 _c = palette_get_entry(get_current_palette(), data & 0xff);
	data = GET_COLOR_INDEX(color);
	uszprintf(buf, size, "%s %d(RGB %d %d %d)",
		  _("Index"),
		  data & 0xff,
		  _rgba_getr(_c),
		  _rgba_getg(_c),
		  _rgba_getb(_c));
	break;
      }

      default:
	assert(FALSE);
	break;
    }
  }
  /* short format */
  else {
    switch (GET_COLOR_TYPE(color)) {

      case COLOR_TYPE_MASK:
	uszprintf(buf, size, "MASK");
	break;

      case COLOR_TYPE_RGB:
	data = GET_COLOR_RGB(color);
	if (imgtype == IMAGE_GRAYSCALE) {
	  uszprintf(buf, size, "KA %02x(%d)",
		    _graya_getv(get_color_for_image(imgtype, color)),
		    _graya_getv(get_color_for_image(imgtype, color)));
	}
	else {
	  uszprintf(buf, size, "RGB %02x%02x%02x",
		    _rgba_getr(data),
		    _rgba_getg(data),
		    _rgba_getb(data));

	  if (imgtype == IMAGE_INDEXED)
	    uszprintf(buf+ustrlen(buf), size, "(%d)",
		      get_color_for_image(imgtype, color));
	}
	break;

      case COLOR_TYPE_HSV:
	data = GET_COLOR_HSV(color);
	if (imgtype == IMAGE_GRAYSCALE) {
	  uszprintf(buf, size, "KA %02x(%d)",
		    _hsva_getv(data),
		    _hsva_getv(data));
	}
	else {
	  uszprintf(buf, size, "HSV %02x%02x%02x",
		    _hsva_geth(data),
		    _hsva_gets(data),
		    _hsva_getv(data));

	  if (imgtype == IMAGE_INDEXED)
	    uszprintf(buf+ustrlen(buf), size, "(%d)",
		      get_color_for_image(imgtype, color));
	}
	break;

      case COLOR_TYPE_GRAY:
	data = GET_COLOR_GRAY(color);
	uszprintf(buf, size, "I %02x (%d)",
		  _graya_getv(data),
		  _graya_getv(data));
	break;

      case COLOR_TYPE_INDEX:
	data = GET_COLOR_INDEX(color);
	uszprintf(buf, size, "I %02x (%d)", data, data);
	break;

      default:
	assert(FALSE);
	break;
    }
  }
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
