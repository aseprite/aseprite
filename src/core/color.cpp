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

#include "jinete/jbase.h"

#include "app.h"
#include "core/color.h"
#include "core/core.h"
#include "modules/gfx.h"
#include "modules/palettes.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "widgets/colbar.h"

/* #define GET_COLOR_TYPE(color)	((ase_uint32)((color).coltype)) */
/* #define GET_COLOR_DATA(color)	((ase_uint32)((color).imgcolor)) */

#define MAKE_COLOR(type,data)		(((type) << 24) | (data))
#define GET_COLOR_TYPE(color)		((color) >> 24)
#define GET_COLOR_DATA(color)		((color) & 0xffffff)
#define GET_COLOR_DATA_RGB(color)	(GET_COLOR_DATA(color) & 0xffffff)
#define GET_COLOR_DATA_HSV(color)	(GET_COLOR_DATA(color) & 0xffffff)
#define GET_COLOR_DATA_GRAY(color)	(GET_COLOR_DATA(color) & 0xff)
#define GET_COLOR_DATA_INDEX(color)	(GET_COLOR_DATA(color) & 0xff)

#define MAKE_DATA(c1,c2,c3)	(((c3) << 16) | ((c2) << 8) | (c1))
#define GET_DATA_C1(c)		(((c) >> 0) & 0xff)
#define GET_DATA_C2(c)		(((c) >> 8) & 0xff)
#define GET_DATA_C3(c)		(((c) >> 16) & 0xff)

static int get_mask_for_bitmap(int depth);

char *color_to_string(color_t color, char *buf, int size)
{
  int data;

  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      uszprintf(buf, size, "mask");
      break;

    case COLOR_TYPE_RGB:
      data = GET_COLOR_DATA_RGB(color);
      uszprintf(buf, size, "rgb{%d,%d,%d}",
		GET_DATA_C1(data),
		GET_DATA_C2(data),
		GET_DATA_C3(data));
      break;

    case COLOR_TYPE_HSV:
      data = GET_COLOR_DATA_HSV(color);
      uszprintf(buf, size, "hsv{%d,%d,%d}",
		GET_DATA_C1(data),
		GET_DATA_C2(data),
		GET_DATA_C3(data));
      break;

    case COLOR_TYPE_GRAY:
      data = GET_COLOR_DATA_GRAY(color);
      uszprintf(buf, size, "gray{%d}", data);
      break;

    case COLOR_TYPE_INDEX:
      data = GET_COLOR_DATA_INDEX(color);
      uszprintf(buf, size, "index{%d}", data);
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
      int c=0, table[3] = { 0, 0, 0 };

      for (tok=ustrtok(str+4, ","); tok;
	   tok=ustrtok(NULL, ",")) {
	if (c < 3)
	  table[c++] = ustrtol(tok, NULL, 10);
      }

      color = color_rgb(table[0], table[1], table[2]);
    } 
    else if (ustrncmp(str, "hsv{", 4) == 0) {
      int c=0, table[3] = { 0, 0, 0 };

      for (tok=ustrtok(str+4, ","); tok;
	   tok=ustrtok(NULL, ",")) {
	if (c < 3)
	  table[c++] = ustrtol(tok, NULL, 10);
      }

      color = color_hsv(table[0], table[1], table[2]);
    } 
    else if (ustrncmp(str, "gray{", 5) == 0) {
      color = color_gray(ustrtol(str+5, NULL, 10));
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

// Returns false only if the color is a index and it is outside the
// valid range (outside the maximum number of colors in the current
// palette)
bool color_is_valid(color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_INDEX: {
      int i = GET_COLOR_DATA_INDEX(color);
      return (i >= 0 && i < get_current_palette()->size());
    }

  }
  return true;
}

bool color_equals(color_t c1, color_t c2)
{
  return
    GET_COLOR_TYPE(c1) == GET_COLOR_TYPE(c2) &&
    GET_COLOR_DATA(c1) == GET_COLOR_DATA(c2);
}

color_t color_mask()
{
  return MAKE_COLOR(COLOR_TYPE_MASK, 0);
}

color_t color_rgb(int r, int g, int b)
{
  return MAKE_COLOR(COLOR_TYPE_RGB,
		    MAKE_DATA(r & 0xff,
			      g & 0xff,
			      b & 0xff));
}

color_t color_hsv(int h, int s, int v)
{
  return MAKE_COLOR(COLOR_TYPE_HSV,
		    MAKE_DATA(h & 0xff,
			      s & 0xff,
			      v & 0xff));
}

color_t color_gray(int g)
{
  return MAKE_COLOR(COLOR_TYPE_GRAY, g & 0xff);
}

color_t color_index(int index)
{
  return MAKE_COLOR(COLOR_TYPE_INDEX, index & 0xff);
}

int color_get_red(color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB:
      return GET_DATA_C1(GET_COLOR_DATA_RGB(color));

    case COLOR_TYPE_HSV: {
      int c = GET_COLOR_DATA_HSV(color);
      int h = GET_DATA_C1(c);
      int s = GET_DATA_C2(c);
      int v = GET_DATA_C3(c);
      hsv_to_rgb_int(&h, &s, &v);
      return h;
    }
      
    case COLOR_TYPE_GRAY:
      return GET_COLOR_DATA_GRAY(color);

    case COLOR_TYPE_INDEX: {
      int i = GET_COLOR_DATA_INDEX(color);
      ASSERT(i >= 0 && i < get_current_palette()->size());

      return _rgba_getr(get_current_palette()->getEntry(i));
    }

  }

  ASSERT(false);
  return -1;
}

int color_get_green(color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB:
      return GET_DATA_C2(GET_COLOR_DATA_RGB(color));

    case COLOR_TYPE_HSV: {
      int c = GET_COLOR_DATA_HSV(color);
      int h = GET_DATA_C1(c);
      int s = GET_DATA_C2(c);
      int v = GET_DATA_C3(c);
      hsv_to_rgb_int(&h, &s, &v);
      return s;
    }
      
    case COLOR_TYPE_GRAY:
      return GET_COLOR_DATA_GRAY(color);

    case COLOR_TYPE_INDEX: {
      int i = GET_COLOR_DATA_INDEX(color);
      ASSERT(i >= 0 && i < get_current_palette()->size());

      return _rgba_getg(get_current_palette()->getEntry(i));
    }

  }

  ASSERT(false);
  return -1;
}

int color_get_blue(color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB:
      return GET_DATA_C3(GET_COLOR_DATA_RGB(color));

    case COLOR_TYPE_HSV: {
      int c = GET_COLOR_DATA_HSV(color);
      int h = GET_DATA_C1(c);
      int s = GET_DATA_C2(c);
      int v = GET_DATA_C3(c);
      hsv_to_rgb_int(&h, &s, &v);
      return v;
    }
      
    case COLOR_TYPE_GRAY:
      return GET_COLOR_DATA_GRAY(color);

    case COLOR_TYPE_INDEX: {
      int i = GET_COLOR_DATA_INDEX(color);
      ASSERT(i >= 0 && i < get_current_palette()->size());

      return _rgba_getb(get_current_palette()->getEntry(i));
    }

  }

  ASSERT(false);
  return -1;
}

int color_get_hue(color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB: {
      int c = GET_COLOR_DATA_RGB(color);
      int r = GET_DATA_C1(c);
      int g = GET_DATA_C2(c);
      int b = GET_DATA_C3(c);
      rgb_to_hsv_int(&r, &g, &b);
      return r;
    }

    case COLOR_TYPE_HSV:
      return GET_DATA_C1(GET_COLOR_DATA_HSV(color));

    case COLOR_TYPE_GRAY:
      return 0;

    case COLOR_TYPE_INDEX: {
      int i = GET_COLOR_DATA_INDEX(color);
      ASSERT(i >= 0 && i < get_current_palette()->size());

      ase_uint32 c = get_current_palette()->getEntry(i);
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return r;
    }

  }

  ASSERT(false);
  return -1;
}

int color_get_saturation(color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB: {
      int c = GET_COLOR_DATA_RGB(color);
      int r = GET_DATA_C1(c);
      int g = GET_DATA_C2(c);
      int b = GET_DATA_C3(c);
      rgb_to_hsv_int(&r, &g, &b);
      return g;
    }

    case COLOR_TYPE_HSV:
      return GET_DATA_C2(GET_COLOR_DATA_HSV(color));

    case COLOR_TYPE_GRAY:
      return 0;

    case COLOR_TYPE_INDEX: {
      int i = GET_COLOR_DATA_INDEX(color);
      ASSERT(i >= 0 && i < get_current_palette()->size());

      ase_uint32 c = get_current_palette()->getEntry(i);
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return g;
    }

  }

  ASSERT(false);
  return -1;
}

int color_get_value(color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB: {
      int c = GET_COLOR_DATA_RGB(color);
      int r = GET_DATA_C1(c);
      int g = GET_DATA_C2(c);
      int b = GET_DATA_C3(c);
      rgb_to_hsv_int(&r, &g, &b);
      return b;
    }

    case COLOR_TYPE_HSV:
      return GET_DATA_C3(GET_COLOR_DATA_HSV(color));
      
    case COLOR_TYPE_GRAY:
      return GET_COLOR_DATA_GRAY(color);

    case COLOR_TYPE_INDEX: {
      int i = GET_COLOR_DATA_INDEX(color);
      ASSERT(i >= 0 && i < get_current_palette()->size());

      ase_uint32 c = get_current_palette()->getEntry(i);
      int r = _rgba_getr(c);
      int g = _rgba_getg(c);
      int b = _rgba_getb(c);
      rgb_to_hsv_int(&r, &g, &b);
      return b;
    }

  }

  ASSERT(false);
  return -1;
}

int color_get_index(color_t color)
{
  switch (GET_COLOR_TYPE(color)) {

    case COLOR_TYPE_MASK:
      return 0;

    case COLOR_TYPE_RGB:
      PRINTF("Getting `index' from a RGB color\n"); /* TODO */
      ASSERT(false);
      break;

    case COLOR_TYPE_HSV:
      PRINTF("Getting `index' from a HSV color\n"); /* TODO */
      ASSERT(false);
      break;

    case COLOR_TYPE_GRAY:
      return GET_COLOR_DATA_GRAY(color);

    case COLOR_TYPE_INDEX:
      return GET_COLOR_DATA_INDEX(color);

  }

  ASSERT(false);
  return -1;
}

color_t color_from_image(int imgtype, int c)
{
  color_t color = color_mask();

  switch (imgtype) {

    case IMAGE_RGB:
      if (_rgba_geta(c) > 0) {
	color = color_rgb(_rgba_getr(c),
			  _rgba_getg(c),
			  _rgba_getb(c));
      }
      break;

    case IMAGE_GRAYSCALE:
      if (_graya_geta(c) > 0) {
	color = color_gray(_graya_getv(c));
      }
      break;

    case IMAGE_INDEXED:
      color = color_index(c);
      break;
  }

  return color;
}

int blackandwhite(int r, int g, int b)
{
  return (r*30+g*59+b*11)/100 < 128 ?
    makecol(0, 0, 0):
    makecol(255, 255, 255);
}

int blackandwhite_neg(int r, int g, int b)
{
  return (r*30+g*59+b*11)/100 < 128 ?
    makecol(255, 255, 255):
    makecol(0, 0, 0);
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
      data = GET_COLOR_DATA_RGB(color);
      c = makeacol_depth(depth,
			 _rgba_getr(data),
			 _rgba_getg(data),
			 _rgba_getb(data), 255);
      break;

    case COLOR_TYPE_HSV: {
      int h, s, v;

      data = GET_COLOR_DATA_HSV(color);
      h = GET_DATA_C1(data);
      s = GET_DATA_C2(data);
      v = GET_DATA_C3(data);
      hsv_to_rgb_int(&h, &s, &v);

      c = makeacol_depth(depth, h, s, v, 255);
      break;
    }
      
    case COLOR_TYPE_GRAY:
      c = GET_COLOR_DATA_GRAY(color);
      if (depth != 8)
	c = makeacol_depth(depth, c, c, c, 255);
      break;

    case COLOR_TYPE_INDEX:
      c = GET_COLOR_DATA_INDEX(color);
      if (depth != 8) {
	ASSERT(c >= 0 && c < (int)get_current_palette()->size());

	ase_uint32 _c = get_current_palette()->getEntry(c);
	c = makeacol_depth(depth,
			   _rgba_getr(_c),
			   _rgba_getg(_c),
			   _rgba_getb(_c), 255);
      }
      break;

  }

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

    case COLOR_TYPE_RGB: {
      int r, g, b;

      data = GET_COLOR_DATA_RGB(color);
      r = GET_DATA_C1(data);
      g = GET_DATA_C2(data);
      b = GET_DATA_C3(data);

      switch (imgtype) {
	case IMAGE_RGB: {
	  c = _rgba(r, g, b, 255);
	  break;
	}
	case IMAGE_GRAYSCALE: {
	  rgb_to_hsv_int(&r, &g, &b);
	  c = _graya(b, 255);
	  break;
	}
	case IMAGE_INDEXED:
	  c = get_current_palette()->findBestfit(r, g, b);
	  break;
      }
      break;
    }

    case COLOR_TYPE_HSV: {
      int h, s, v;

      data = GET_COLOR_DATA_HSV(color);
      h = GET_DATA_C1(data);
      s = GET_DATA_C2(data);
      v = GET_DATA_C3(data);
      
      switch (imgtype) {
	case IMAGE_RGB:
	  hsv_to_rgb_int(&h, &s, &v);
	  c = _rgba(h, s, v, 255);
	  break;
	case IMAGE_GRAYSCALE: {
	  c = _graya(v, 255);
	  break;
	}
	case IMAGE_INDEXED:
	  hsv_to_rgb_int(&h, &s, &v);
	  c = get_current_palette()->findBestfit(h, s, v);
	  break;
      }
      break;
    }
      
    case COLOR_TYPE_GRAY:
      data = GET_COLOR_DATA_GRAY(color);
      switch (imgtype) {
	case IMAGE_RGB:
	  c = data;
	  c = _rgba(c, c, c, 255);
	  break;
	case IMAGE_GRAYSCALE:
	  c = data;
	  break;
	case IMAGE_INDEXED:
	  c = data;
	  c = get_current_palette()->findBestfit(c, c, c);
	  break;
      }
      break;

    case COLOR_TYPE_INDEX:
      data = GET_COLOR_DATA_INDEX(color);
      switch (imgtype) {
	case IMAGE_RGB: {
	  ase_uint32 _c = get_current_palette()->getEntry(data);
	  c = _rgba(_rgba_getr(_c),
		    _rgba_getg(_c),
		    _rgba_getb(_c), 255);
	  break;
	}
	case IMAGE_GRAYSCALE:
	  c = _graya(data & 0xff, 255);
	  break;
	case IMAGE_INDEXED:
	  c = MID(0, (data & 0xff), get_current_palette()->size()-1);
	  break;
      }
      break;

  }

  return c;
}

int get_color_for_layer(Layer *layer, color_t color)
{
  return
    fixup_color_for_layer(layer,
			  get_color_for_image(layer->getSprite()->getImgType(),
					      color));
}

int fixup_color_for_layer(Layer *layer, int color)
{
  if (layer->is_background())
    return fixup_color_for_background(layer->getSprite()->getImgType(), color);
  else
    return color;
}

int fixup_color_for_background(int imgtype, int color)
{
  switch (imgtype) {
    case IMAGE_RGB:
      if (_rgba_geta(color) < 255) {
	return _rgba(_rgba_getr(color),
		     _rgba_getg(color),
		     _rgba_getb(color), 255);
      }
      break;
    case IMAGE_GRAYSCALE:
      if (_graya_geta(color) < 255) {
	return _graya(_graya_getv(color), 255);
      }
      break;
  }
  return color;
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

  // Long format
  if (long_format) {
    switch (GET_COLOR_TYPE(color)) {

      case COLOR_TYPE_MASK:
	ustrncpy(buf, _("Mask"), size);
	break;

      case COLOR_TYPE_RGB:
	data = GET_COLOR_DATA_RGB(color);
	if (imgtype == IMAGE_GRAYSCALE) {
	  uszprintf(buf, size, "Gray %d",
		    _graya_getv(get_color_for_image(imgtype, color)));
	}
	else {
	  uszprintf(buf, size, "RGB %d %d %d",
		    GET_DATA_C1(data),
		    GET_DATA_C2(data),
		    GET_DATA_C3(data));
	  
	  if (imgtype == IMAGE_INDEXED)
	    uszprintf(buf+ustrlen(buf), size, " %s %d",
		      _("Index"), get_color_for_image(imgtype, color));
	}
	break;

      case COLOR_TYPE_HSV:
	data = GET_COLOR_DATA_HSV(color);
	if (imgtype == IMAGE_GRAYSCALE) {
	  uszprintf(buf, size, "Gray %d",
		    GET_DATA_C3(data));
	}
	else {
	  uszprintf(buf, size, "HSV %d %d %d",
		    GET_DATA_C1(data),
		    GET_DATA_C2(data),
		    GET_DATA_C3(data));
  
	  if (imgtype == IMAGE_INDEXED)
	    uszprintf(buf+ustrlen(buf), size, " Index %d",
		      get_color_for_image(imgtype, color));
	}
	break;
	
      case COLOR_TYPE_GRAY:
	data = GET_COLOR_DATA_GRAY(color);
	uszprintf(buf, size, "Gray %d",
		  data);
	break;

      case COLOR_TYPE_INDEX:
	data = GET_COLOR_DATA_INDEX(color);
	if (data >= 0 && data < (int)get_current_palette()->size()) {
	  ase_uint32 _c = get_current_palette()->getEntry(data);
	  uszprintf(buf, size, "Index %d (RGB %d %d %d)",
		    data,
		    _rgba_getr(_c),
		    _rgba_getg(_c),
		    _rgba_getb(_c));
	}
	else {
	  uszprintf(buf, size, "Index %d (out of range)", data);
	}
	break;

      default:
	ASSERT(false);
	break;
    }
  }
  // Short format
  else {
    switch (GET_COLOR_TYPE(color)) {

      case COLOR_TYPE_MASK:
	uszprintf(buf, size, "MASK");
	break;

      case COLOR_TYPE_RGB:
	data = GET_COLOR_DATA_RGB(color);
	if (imgtype == IMAGE_GRAYSCALE) {
	  uszprintf(buf, size, "V %d",
		    _graya_getv(get_color_for_image(imgtype, color)));
	}
	else {
	  uszprintf(buf, size, "RGB %02x%02x%02x",
		    GET_DATA_C1(data),
		    GET_DATA_C2(data),
		    GET_DATA_C3(data));

	  if (imgtype == IMAGE_INDEXED)
	    uszprintf(buf+ustrlen(buf), size, "(%d)",
		      get_color_for_image(imgtype, color));
	}
	break;

      case COLOR_TYPE_HSV:
	data = GET_COLOR_DATA_HSV(color);
	if (imgtype == IMAGE_GRAYSCALE) {
	  uszprintf(buf, size, "V %d",
		    GET_DATA_C3(data));
	}
	else {
	  uszprintf(buf, size, "HSV %02x%02x%02x",
		    GET_DATA_C1(data),
		    GET_DATA_C2(data),
		    GET_DATA_C3(data));

	  if (imgtype == IMAGE_INDEXED)
	    uszprintf(buf+ustrlen(buf), size, "(%d)",
		      get_color_for_image(imgtype, color));
	}
	break;

      case COLOR_TYPE_GRAY:
	data = GET_COLOR_DATA_GRAY(color);
	uszprintf(buf, size, "V %d", data);
	break;

      case COLOR_TYPE_INDEX:
	data = GET_COLOR_DATA_INDEX(color);
	uszprintf(buf, size, "I %d", data);
	break;

      default:
	ASSERT(false);
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
