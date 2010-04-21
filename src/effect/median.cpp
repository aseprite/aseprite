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

#include "effect/effect.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"
#include "tiled_mode.h"

static struct {
  TiledMode tiled;
  int w, h;
  int ncolors;
  unsigned char *channel[4];
} data = { TILED_NONE, 0, 0, 0, { NULL, NULL, NULL, NULL } };

void set_median_size(TiledMode tiled, int w, int h)
{
  int c;

  data.tiled = tiled;
  data.w = w;
  data.h = h;
  data.ncolors = w*h;

  for (c=0; c<4; c++) {
    if (data.channel[c])
      jfree(data.channel[c]);

    data.channel[c] = (unsigned char*)jmalloc(sizeof(unsigned char) * data.ncolors);
  }
}

static int cmp_channel(const void *p1, const void *p2)
{
  return (*(unsigned char *)p2) -(*(unsigned char *)p1);
}

void apply_median4(Effect *effect)
{
  const Image *src = effect->src;
  Image *dst = effect->dst;
  const ase_uint32 *src_address;
  ase_uint32 *dst_address;
  int x, y, dx, dy, color;
  int c, w, r, g, b, a;
  int getx, gety, addx, addy;

  w = effect->x+effect->w;
  y = effect->y+effect->row;
  dst_address = ((ase_uint32 **)dst->line)[y]+effect->x;

  for (x=effect->x; x<w; x++) {
    /* avoid the unmask region */
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	dst_address++;
	_image_bitmap_next_bit(effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit(effect->d, effect->mask_address);
    }

    c = 0;

    GET_MATRIX_DATA
      (ase_uint32, src, src_address,
       data.w, data.h, data.w/2, data.h/2, data.tiled,
       color = *src_address;
       data.channel[0][c] = _rgba_getr(color);
       data.channel[1][c] = _rgba_getg(color);
       data.channel[2][c] = _rgba_getb(color);
       data.channel[3][c] = _rgba_geta(color);
       c++;
       );

    for (c=0; c<4; c++)
      qsort(data.channel[c], data.ncolors, sizeof(unsigned char), cmp_channel);

    color = image_getpixel_fast<RgbTraits>(src, x, y);

    if (effect->target & TARGET_RED_CHANNEL)
      r = data.channel[0][data.ncolors/2];
    else
      r = _rgba_getr(color);

    if (effect->target & TARGET_GREEN_CHANNEL)
      g = data.channel[1][data.ncolors/2];
    else
      g = _rgba_getg(color);

    if (effect->target & TARGET_BLUE_CHANNEL)
      b = data.channel[2][data.ncolors/2];
    else
      b = _rgba_getb(color);

    if (effect->target & TARGET_ALPHA_CHANNEL)
      a = data.channel[3][data.ncolors/2];
    else
      a = _rgba_geta(color);

    *(dst_address++) = _rgba(r, g, b, a);
  }
}

void apply_median2(Effect *effect)
{
  const Image *src = effect->src;
  Image *dst = effect->dst;
  const ase_uint16 *src_address;
  ase_uint16 *dst_address;
  int x, y, dx, dy, color;
  int c, w, k, a;
  int getx, gety, addx, addy;

  w = effect->x+effect->w;
  y = effect->y+effect->row;
  dst_address = ((ase_uint16 **)dst->line)[y]+effect->x;

  for (x=effect->x; x<w; x++) {
    /* avoid the unmask region */
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	dst_address++;
	_image_bitmap_next_bit(effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit(effect->d, effect->mask_address);
    }

    c = 0;

    GET_MATRIX_DATA
      (ase_uint16, src, src_address,
       data.w, data.h, data.w/2, data.h/2, data.tiled,
       color = *src_address;
       data.channel[0][c] = _graya_getv(color);
       data.channel[1][c] = _graya_geta(color);
       c++;
       );

    for (c=0; c<2; c++)
      qsort(data.channel[c], data.ncolors, sizeof(unsigned char), cmp_channel);

    color = image_getpixel_fast<GrayscaleTraits>(src, x, y);

    if (effect->target & TARGET_GRAY_CHANNEL)
      k = data.channel[0][data.ncolors/2];
    else
      k = _graya_getv(color);

    if (effect->target & TARGET_ALPHA_CHANNEL)
      a = data.channel[1][data.ncolors/2];
    else
      a = _graya_geta(color);

    *(dst_address++) = _graya(k, a);
  }
}

void apply_median1(Effect *effect)
{
  const Palette* pal = get_current_palette();
  const RgbMap* rgbmap = effect->sprite->getRgbMap();
  const Image* src = effect->src;
  Image* dst = effect->dst;
  const ase_uint8* src_address;
  ase_uint8* dst_address;
  int x, y, dx, dy, color;
  int c, w, r, g, b;
  int getx, gety, addx, addy;

  w = effect->x+effect->w;
  y = effect->y+effect->row;
  dst_address = ((ase_uint8 **)dst->line)[y]+effect->x;

  for (x=effect->x; x<w; x++) {
    /* avoid the unmask region */
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	dst_address++;
	_image_bitmap_next_bit(effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit(effect->d, effect->mask_address);
    }

    c = 0;

    GET_MATRIX_DATA
      (ase_uint8, src, src_address,
       data.w, data.h, data.w/2, data.h/2, data.tiled,
       color = *src_address;
       if (effect->target & TARGET_INDEX_CHANNEL) {
	 data.channel[0][c] = color;
       }
       else {
	 data.channel[0][c] = _rgba_getr(pal->getEntry(color));
	 data.channel[1][c] = _rgba_getg(pal->getEntry(color));
	 data.channel[2][c] = _rgba_getb(pal->getEntry(color));
       }
       c++;
       );

    if (effect->target & TARGET_INDEX_CHANNEL)
      qsort(data.channel[0], data.ncolors, sizeof(unsigned char), cmp_channel);
    else {
      for (c=0; c<3; c++)
	qsort(data.channel[c], data.ncolors, sizeof(unsigned char), cmp_channel);
    }

    if (effect->target & TARGET_INDEX_CHANNEL) {
      *(dst_address++) = data.channel[0][data.ncolors/2];
    }
    else {
      color = image_getpixel_fast<IndexedTraits>(src, x, y);

      if (effect->target & TARGET_RED_CHANNEL)
	r = data.channel[0][data.ncolors/2];
      else
	r = _rgba_getr(pal->getEntry(color));

      if (effect->target & TARGET_GREEN_CHANNEL)
	g = data.channel[1][data.ncolors/2];
      else
	g = _rgba_getg(pal->getEntry(color));

      if (effect->target & TARGET_BLUE_CHANNEL)
	b = data.channel[2][data.ncolors/2];
      else
	b = _rgba_getb(pal->getEntry(color));

      *(dst_address++) = rgbmap->mapColor(r, g, b);
    }
  }
}
