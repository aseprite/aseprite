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

#ifndef CORE_COLOR_H
#define CORE_COLOR_H

#include "jinete/jbase.h"

struct BITMAP;
struct Image;

enum {
  COLOR_TYPE_MASK,
  COLOR_TYPE_RGB,
  COLOR_TYPE_HSV,
  COLOR_TYPE_GRAY,
  COLOR_TYPE_INDEX,
};

typedef struct color_t
{
  ase_uint32 coltype;
  ase_uint32 imgcolor;
} color_t;

/* typedef uint64_t color_t; */

char *color_to_string(color_t color, char *buf, int size);
color_t string_to_color(const char *str);

int color_type(color_t color);
bool color_equals(color_t c1, color_t c2);

color_t color_mask(void);
color_t color_rgb(int r, int g, int b, int a);
color_t color_hsv(int h, int s, int v, int a);
color_t color_gray(int g, int a);
color_t color_index(int index);
int color_get_red(int imgtype, color_t color);
int color_get_green(int imgtype, color_t color);
int color_get_blue(int imgtype, color_t color);
int color_get_hue(int imgtype, color_t color);
int color_get_saturation(int imgtype, color_t color);
int color_get_value(int imgtype, color_t color);
int color_get_index(int imgtype, color_t color);
int color_get_alpha(int imgtype, color_t color);
void color_set_alpha(color_t *color, int alpha);
color_t color_from_image(int imgtype, int c);

int blackandwhite(int r, int g, int b);
int blackandwhite_neg(int r, int g, int b);

int get_color_for_allegro(int depth, color_t color);
int get_color_for_image(int imgtype, color_t color);
color_t image_getpixel_color(struct Image *image, int x, int y);
void color_to_formalstring(int imgtype, color_t color, char *buf,
			   int size, bool long_format);

#endif /* MODULES_COLOR_H */

