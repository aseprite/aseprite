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

#include "effect/effect.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"

void apply_invert_color4(Effect *effect)
{
  ase_uint32 *src_address;
  ase_uint32 *dst_address;
  int x, c, r, g, b, a;

  src_address = ((ase_uint32 **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((ase_uint32 **)effect->dst->line)[effect->row+effect->y]+effect->x;

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

    r = _rgba_getr(c);
    g = _rgba_getg(c);
    b = _rgba_getb(c);
    a = _rgba_geta(c);

    if (effect->target & TARGET_RED_CHANNEL) r ^= 0xff;
    if (effect->target & TARGET_GREEN_CHANNEL) g ^= 0xff;
    if (effect->target & TARGET_BLUE_CHANNEL) b ^= 0xff;
    if (effect->target & TARGET_ALPHA_CHANNEL) a ^= 0xff;

    *(dst_address++) = _rgba(r, g, b, a);
  }
}

void apply_invert_color2(Effect *effect)
{
  ase_uint16 *src_address;
  ase_uint16 *dst_address;
  int x, c, k, a;

  src_address = ((ase_uint16 **)effect->src->line)[effect->row+effect->y]+effect->x;
  dst_address = ((ase_uint16 **)effect->dst->line)[effect->row+effect->y]+effect->x;

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

    k = _graya_getv(c);
    a = _graya_geta(c);

    if (effect->target & TARGET_GRAY_CHANNEL) k ^= 0xff;
    if (effect->target & TARGET_ALPHA_CHANNEL) a ^= 0xff;

    *(dst_address++) = _graya(k, a);
  }
}

void apply_invert_color1(Effect *effect)
{
  Palette *pal = get_current_palette();
  RgbMap* rgbmap = effect->sprite->getRgbMap();
  ase_uint8 *src_address;
  ase_uint8 *dst_address;
  int x, c, r, g, b;

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

    if (effect->target & TARGET_INDEX_CHANNEL)
      c ^= 0xff;
    else {
      r = _rgba_getr(pal->getEntry(c));
      g = _rgba_getg(pal->getEntry(c));
      b = _rgba_getb(pal->getEntry(c));

      if (effect->target & TARGET_RED_CHANNEL  ) r ^= 0xff;
      if (effect->target & TARGET_GREEN_CHANNEL) g ^= 0xff;
      if (effect->target & TARGET_BLUE_CHANNEL ) b ^= 0xff;

      c = rgbmap->mapColor(r, g, b);
    }

    *(dst_address++) = c;
  }
}
