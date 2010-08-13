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

#include "core/color.h"
#include "effect/effect.h"
#include "raster/image.h"

static struct {
  int from, to;
  int tolerance;
} data;

void set_replace_colors(int from, int to, int tolerance)
{
  data.from = from;
  data.to = to;
  data.tolerance = MID(0, tolerance, 255);
}

void apply_replace_color4(Effect *effect)
{
  ase_uint32 *src_address;
  ase_uint32 *dst_address;
  int src_r, src_g, src_b, src_a;
  int dst_r, dst_g, dst_b, dst_a;
  int x, c;

  src_address = ((ase_uint32 **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((ase_uint32 **)effect->dst->line)[effect->row+effect->y]+effect->x;

  dst_r = _rgba_getr(data.from);
  dst_g = _rgba_getg(data.from);
  dst_b = _rgba_getb(data.from);
  dst_a = _rgba_geta(data.from);

  for (x=0; x<effect->w; x++) {
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	src_address++;
	dst_address++;
	_image_bitmap_next_bit(effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit(effect->d, effect->mask_address);
    }

    c = *(src_address++);

    src_r = _rgba_getr(c);
    src_g = _rgba_getg(c);
    src_b = _rgba_getb(c);
    src_a = _rgba_geta(c);

    if ((ABS(src_r-dst_r) <= data.tolerance) &&
        (ABS(src_g-dst_g) <= data.tolerance) &&
        (ABS(src_b-dst_b) <= data.tolerance) &&
        (ABS(src_a-dst_a) <= data.tolerance))
      *(dst_address++) = data.to;
    else
      *(dst_address++) = c;
  }
}

void apply_replace_color2(Effect *effect)
{
  ase_uint16 *src_address;
  ase_uint16 *dst_address;
  int src_k, src_a;
  int dst_k, dst_a;
  int x, c;

  src_address = ((ase_uint16 **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((ase_uint16 **)effect->dst->line)[effect->row+effect->y]+effect->x;

  dst_k = _graya_getv(data.from);
  dst_a = _graya_geta(data.from);

  for (x=0; x<effect->w; x++) {
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	src_address++;
	dst_address++;
	_image_bitmap_next_bit(effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit(effect->d, effect->mask_address);
    }

    c = *(src_address++);

    src_k = _graya_getv(c);
    src_a = _graya_geta(c);

    if ((ABS(src_k-dst_k) <= data.tolerance) &&
        (ABS(src_a-dst_a) <= data.tolerance))
      *(dst_address++) = data.to;
    else
      *(dst_address++) = c;
  }
}

void apply_replace_color1(Effect *effect)
{
  ase_uint8 *src_address;
  ase_uint8 *dst_address;
  int x, c;

  src_address = ((ase_uint8 **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((ase_uint8 **)effect->dst->line)[effect->row+effect->y]+effect->x;

  for (x=0; x<effect->w; x++) {
    if (effect->mask_address) {
      if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	src_address++;
	dst_address++;
	_image_bitmap_next_bit(effect->d, effect->mask_address);
	continue;
      }
      else
	_image_bitmap_next_bit(effect->d, effect->mask_address);
    }

    c = *(src_address++);

    if (ABS(c-data.from) <= data.tolerance)
      *(dst_address++) = data.to;
    else
      *(dst_address++) = c;
  }
}
