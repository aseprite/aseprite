/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "filters/replace_color_filter.h"

#include "filters/filter_manager.h"
#include "raster/image.h"

ReplaceColorFilter::ReplaceColorFilter()
{
  m_from = m_to = m_tolerance = 0;
}

void ReplaceColorFilter::setFrom(int from)
{
  m_from = from;
}

void ReplaceColorFilter::setTo(int to)
{
  m_to = to;
}

void ReplaceColorFilter::setTolerance(int tolerance)
{
  m_tolerance = MID(0, tolerance, 255);
}

const char* ReplaceColorFilter::getName()
{
  return "Replace Color";
}

void ReplaceColorFilter::applyToRgba(FilterManager* filterMgr)
{
  const uint32_t* src_address = (uint32_t*)filterMgr->getSourceAddress();
  uint32_t* dst_address = (uint32_t*)filterMgr->getDestinationAddress();
  int w = filterMgr->getWidth();
  int src_r, src_g, src_b, src_a;
  int dst_r, dst_g, dst_b, dst_a;
  int x, c;

  dst_r = _rgba_getr(m_from);
  dst_g = _rgba_getg(m_from);
  dst_b = _rgba_getb(m_from);
  dst_a = _rgba_geta(m_from);

  for (x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    c = *(src_address++);

    src_r = _rgba_getr(c);
    src_g = _rgba_getg(c);
    src_b = _rgba_getb(c);
    src_a = _rgba_geta(c);

    if ((ABS(src_r-dst_r) <= m_tolerance) &&
        (ABS(src_g-dst_g) <= m_tolerance) &&
        (ABS(src_b-dst_b) <= m_tolerance) &&
        (ABS(src_a-dst_a) <= m_tolerance))
      *(dst_address++) = m_to;
    else
      *(dst_address++) = c;
  }
}

void ReplaceColorFilter::applyToGrayscale(FilterManager* filterMgr)
{
  const uint16_t* src_address = (uint16_t*)filterMgr->getSourceAddress();
  uint16_t* dst_address = (uint16_t*)filterMgr->getDestinationAddress();
  int w = filterMgr->getWidth();
  int src_k, src_a;
  int dst_k, dst_a;
  int x, c;

  dst_k = _graya_getv(m_from);
  dst_a = _graya_geta(m_from);

  for (x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    c = *(src_address++);

    src_k = _graya_getv(c);
    src_a = _graya_geta(c);

    if ((ABS(src_k-dst_k) <= m_tolerance) &&
        (ABS(src_a-dst_a) <= m_tolerance))
      *(dst_address++) = m_to;
    else
      *(dst_address++) = c;
  }
}

void ReplaceColorFilter::applyToIndexed(FilterManager* filterMgr)
{
  const uint8_t* src_address = (uint8_t*)filterMgr->getSourceAddress();
  uint8_t* dst_address = (uint8_t*)filterMgr->getDestinationAddress();
  int w = filterMgr->getWidth();
  int x, c;

  for (x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    c = *(src_address++);

    if (ABS(c-m_from) <= m_tolerance)
      *(dst_address++) = m_to;
    else
      *(dst_address++) = c;
  }
}
