/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#include "effect/effect.h"
#include "modules/palette.h"
#include "raster/image.h"

#endif

void apply_invert_color4 (Effect *effect)
{
  unsigned long *src_address;
  unsigned long *dst_address;
  int x, c, r, g, b, a;

  src_address = ((unsigned long **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((unsigned long **)effect->dst->line)[effect->row+effect->y]+effect->x;

  for (x=0; x<effect->w; x++) {
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	src_address++;
	dst_address++;
	_image_bitmap_next_bit (effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit (effect->d, effect->mask_address);
    }

    c = *(src_address++);

    r = _rgba_getr (c);
    g = _rgba_getg (c);
    b = _rgba_getb (c);
    a = _rgba_geta (c);

    if (effect->target.r) r ^= 0xff;
    if (effect->target.g) g ^= 0xff;
    if (effect->target.b) b ^= 0xff;
    if (effect->target.a) a ^= 0xff;

    *(dst_address++) = _rgba (r, g, b, a);
  }
}

void apply_invert_color2 (Effect *effect)
{
  unsigned short *src_address;
  unsigned short *dst_address;
  int x, c, k, a;

  src_address = ((unsigned short **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((unsigned short **)effect->dst->line)[effect->row+effect->y]+effect->x;

  for (x=0; x<effect->w; x++) {
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	src_address++;
	dst_address++;
	_image_bitmap_next_bit (effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit (effect->d, effect->mask_address);
    }

    c = *(src_address++);

    k = _graya_getk (c);
    a = _graya_geta (c);

    if (effect->target.k) k ^= 0xff;
    if (effect->target.a) a ^= 0xff;

    *(dst_address++) = _graya (k, a);
  }
}

void apply_invert_color1 (Effect *effect)
{
  unsigned char *src_address;
  unsigned char *dst_address;
  int x, c, r, g, b;

  src_address = ((unsigned char **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((unsigned char **)effect->dst->line)[effect->row+effect->y]+effect->x;

  for (x=0; x<effect->w; x++) {
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	src_address++;
	dst_address++;
	_image_bitmap_next_bit (effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit (effect->d, effect->mask_address);
    }

    c = *(src_address++);

    if (effect->target.index)
      c ^= 0xff;
    else {
      r = (current_palette[c].r>>1);
      g = (current_palette[c].g>>1);
      b = (current_palette[c].b>>1);

      if (effect->target.r) r ^= 0x1f;
      if (effect->target.g) g ^= 0x1f;
      if (effect->target.b) b ^= 0x1f;

      c = orig_rgb_map->data[r][g][b];
    }

    *(dst_address++) = c;
  }
}
