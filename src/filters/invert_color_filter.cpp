/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include "filters/invert_color_filter.h"

#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"

const char* InvertColorFilter::getName()
{
  return "Invert Color";
}

void InvertColorFilter::applyToRgba(FilterManager* filterMgr)
{
  const uint32_t* src_address = (uint32_t*)filterMgr->getSourceAddress();
  uint32_t* dst_address = (uint32_t*)filterMgr->getDestinationAddress();
  int w = filterMgr->getWidth();
  Target target = filterMgr->getTarget();
  int x, c, r, g, b, a;

  for (x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    c = *(src_address++);

    r = _rgba_getr(c);
    g = _rgba_getg(c);
    b = _rgba_getb(c);
    a = _rgba_geta(c);

    if (target & TARGET_RED_CHANNEL) r ^= 0xff;
    if (target & TARGET_GREEN_CHANNEL) g ^= 0xff;
    if (target & TARGET_BLUE_CHANNEL) b ^= 0xff;
    if (target & TARGET_ALPHA_CHANNEL) a ^= 0xff;

    *(dst_address++) = _rgba(r, g, b, a);
  }
}

void InvertColorFilter::applyToGrayscale(FilterManager* filterMgr)
{
  const uint16_t* src_address = (uint16_t*)filterMgr->getSourceAddress();
  uint16_t* dst_address = (uint16_t*)filterMgr->getDestinationAddress();
  int w = filterMgr->getWidth();
  Target target = filterMgr->getTarget();
  int x, c, k, a;

  for (x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    c = *(src_address++);

    k = _graya_getv(c);
    a = _graya_geta(c);

    if (target & TARGET_GRAY_CHANNEL) k ^= 0xff;
    if (target & TARGET_ALPHA_CHANNEL) a ^= 0xff;

    *(dst_address++) = _graya(k, a);
  }
}

void InvertColorFilter::applyToIndexed(FilterManager* filterMgr)
{
  const uint8_t* src_address = (uint8_t*)filterMgr->getSourceAddress();
  uint8_t* dst_address = (uint8_t*)filterMgr->getDestinationAddress();
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
  int w = filterMgr->getWidth();
  Target target = filterMgr->getTarget();
  int x, c, r, g, b;

  for (x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    c = *(src_address++);

    if (target & TARGET_INDEX_CHANNEL)
      c ^= 0xff;
    else {
      r = _rgba_getr(pal->getEntry(c));
      g = _rgba_getg(pal->getEntry(c));
      b = _rgba_getb(pal->getEntry(c));

      if (target & TARGET_RED_CHANNEL  ) r ^= 0xff;
      if (target & TARGET_GREEN_CHANNEL) g ^= 0xff;
      if (target & TARGET_BLUE_CHANNEL ) b ^= 0xff;

      c = rgbmap->mapColor(r, g, b);
    }

    *(dst_address++) = c;
  }
}
