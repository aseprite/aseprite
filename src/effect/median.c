/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#include "effect/effect.h"
#include "modules/palette.h"
#include "modules/tools.h"
#include "raster/image.h"

#endif

static struct {
  int tiled;
  int w, h;
  int ncolors;
  unsigned char *channel[4];
} data = { FALSE, 0, 0, 0, { NULL, NULL, NULL, NULL } };

void set_median_size (int w, int h)
{
  int c;

  data.tiled = get_tiled_mode ();
  data.w = w;
  data.h = h;
  data.ncolors = w*h;

  for (c=0; c<4; c++) {
    if (data.channel[c])
      jfree (data.channel[c]);

    data.channel[c] = jmalloc (sizeof (unsigned char) * data.ncolors);
  }
}

static int cmp_channel (const void *p1, const void *p2)
{
  return (*(unsigned char *)p2) - (*(unsigned char *)p1);
}

void apply_median4 (Effect *effect)
{
  Image *src = effect->src;
  Image *dst = effect->dst;
  unsigned long *src_address;
  unsigned long *dst_address;
  int x, y, dx, dy, color;
  int c, w, r, g, b, a;
  int getx, gety, addx, addy;

  w = effect->x+effect->w;
  y = effect->y+effect->row;
  dst_address = ((unsigned long **)dst->line)[y]+effect->x;

  for (x=effect->x; x<w; x++) {
    /* avoid the unmask region */
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	dst_address++;
	_image_bitmap_next_bit (effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit (effect->d, effect->mask_address);
    }

    c = 0;

    GET_MATRIX_DATA (unsigned long, data.w, data.h, data.w/2, data.h/2,
		     color = *src_address;
		     data.channel[0][c] = _rgba_getr (color);
		     data.channel[1][c] = _rgba_getg (color);
		     data.channel[2][c] = _rgba_getb (color);
		     data.channel[3][c] = _rgba_geta (color);
		     c++;
		     );

    for (c=0; c<4; c++)
      qsort (data.channel[c], data.ncolors, sizeof (unsigned char), cmp_channel);

    color = src->method->getpixel (src, x, y);

    if (effect->target.r)
      r = data.channel[0][data.ncolors/2];
    else
      r = _rgba_getr (color);

    if (effect->target.g)
      g = data.channel[1][data.ncolors/2];
    else
      g = _rgba_getg (color);

    if (effect->target.b)
      b = data.channel[2][data.ncolors/2];
    else
      b = _rgba_getb (color);

    if (effect->target.a)
      a = data.channel[3][data.ncolors/2];
    else
      a = _rgba_geta (color);

    *(dst_address++) = _rgba (r, g, b, a);
  }
}

void apply_median2 (Effect *effect)
{
  Image *src = effect->src;
  Image *dst = effect->dst;
  unsigned short *src_address;
  unsigned short *dst_address;
  int x, y, dx, dy, color;
  int c, w, k, a;
  int getx, gety, addx, addy;

  w = effect->x+effect->w;
  y = effect->y+effect->row;
  dst_address = ((unsigned short **)dst->line)[y]+effect->x;

  for (x=effect->x; x<w; x++) {
    /* avoid the unmask region */
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	dst_address++;
	_image_bitmap_next_bit (effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit (effect->d, effect->mask_address);
    }

    c = 0;

    GET_MATRIX_DATA (unsigned short, data.w, data.h, data.w/2, data.h/2,
		     color = *src_address;
		     data.channel[0][c] = _graya_getk (color);
		     data.channel[1][c] = _graya_geta (color);
		     c++;
		     );

    for (c=0; c<2; c++)
      qsort (data.channel[c], data.ncolors, sizeof (unsigned char), cmp_channel);

    color = src->method->getpixel (src, x, y);

    if (effect->target.k)
      k = data.channel[0][data.ncolors/2];
    else
      k = _graya_getk (color);

    if (effect->target.a)
      a = data.channel[1][data.ncolors/2];
    else
      a = _graya_geta (color);

    *(dst_address++) = _graya (k, a);
  }
}

void apply_median1 (Effect *effect)
{
  Image *src = effect->src;
  Image *dst = effect->dst;
  unsigned char *src_address;
  unsigned char *dst_address;
  int x, y, dx, dy, color;
  int c, w, r, g, b;
  int getx, gety, addx, addy;

  w = effect->x+effect->w;
  y = effect->y+effect->row;
  dst_address = ((unsigned char **)dst->line)[y]+effect->x;

  for (x=effect->x; x<w; x++) {
    /* avoid the unmask region */
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	dst_address++;
	_image_bitmap_next_bit (effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit (effect->d, effect->mask_address);
    }

    c = 0;

    GET_MATRIX_DATA (unsigned char, data.w, data.h, data.w/2, data.h/2,
		     color = *src_address;
		     if (effect->target.index) {
		       data.channel[0][c] = color;
		     }
		     else {
		       data.channel[0][c] = _rgb_scale_6[current_palette[color].r];
		       data.channel[1][c] = _rgb_scale_6[current_palette[color].g];
		       data.channel[2][c] = _rgb_scale_6[current_palette[color].b];
		     }
		     c++;
		     );

    if (effect->target.index)
      qsort (data.channel[0], data.ncolors, sizeof (unsigned char), cmp_channel);
    else {
      for (c=0; c<3; c++)
	qsort (data.channel[c], data.ncolors, sizeof (unsigned char), cmp_channel);
    }

    if (effect->target.index) {
      *(dst_address++) = data.channel[0][data.ncolors/2];
    }
    else {
      color = src->method->getpixel (src, x, y);

      if (effect->target.r)
	r = data.channel[0][data.ncolors/2];
      else
	r = _rgb_scale_6[current_palette[color].r];

      if (effect->target.g)
	g = data.channel[1][data.ncolors/2];
      else
	g = _rgb_scale_6[current_palette[color].g];

      if (effect->target.b)
	b = data.channel[2][data.ncolors/2];
      else
	b = _rgb_scale_6[current_palette[color].b];

      *(dst_address++) = orig_rgb_map->data[r>>3][g>>3][b>>3];
    }
  }
}
