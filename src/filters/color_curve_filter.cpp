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

#include "filters/color_curve_filter.h"

#include <vector>

#include "filters/color_curve.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"
#include "raster/sprite.h"

ColorCurveFilter::ColorCurveFilter()
  : m_curve(NULL)
  , m_cmap(256)
{
}

void ColorCurveFilter::setCurve(ColorCurve* curve)
{
  ASSERT(curve != NULL);

  m_curve = curve;

  // Generate the color convertion map
  m_curve->getValues(0, 255, m_cmap);
  for (int c=0; c<256; c++)
    m_cmap[c] = MID(0, m_cmap[c], 255);
}

const char* ColorCurveFilter::getName()
{
  return "Color Curve";
}

void ColorCurveFilter::applyToRgba(FilterManager* filterMgr)
{
  const ase_uint32* src_address = (ase_uint32*)filterMgr->getSourceAddress();
  ase_uint32* dst_address = (ase_uint32*)filterMgr->getDestinationAddress();
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

    if (target & TARGET_RED_CHANNEL) r = m_cmap[r];
    if (target & TARGET_GREEN_CHANNEL) g = m_cmap[g];
    if (target & TARGET_BLUE_CHANNEL) b = m_cmap[b];
    if (target & TARGET_ALPHA_CHANNEL) a = m_cmap[a];

    *(dst_address++) = _rgba(r, g, b, a);
  }
}

void ColorCurveFilter::applyToGrayscale(FilterManager* filterMgr)
{
  const ase_uint16* src_address = (ase_uint16*)filterMgr->getSourceAddress();
  ase_uint16* dst_address = (ase_uint16*)filterMgr->getDestinationAddress();
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

    if (target & TARGET_GRAY_CHANNEL) k = m_cmap[k];
    if (target & TARGET_ALPHA_CHANNEL) a = m_cmap[a];

    *(dst_address++) = _graya(k, a);
  }
}

void ColorCurveFilter::applyToIndexed(FilterManager* filterMgr)
{
  const ase_uint8* src_address = (ase_uint8*)filterMgr->getSourceAddress();
  ase_uint8* dst_address = (ase_uint8*)filterMgr->getDestinationAddress();
  int w = filterMgr->getWidth();
  Target target = filterMgr->getTarget();
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
  int x, c, r, g, b;

  for (x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    c = *(src_address++);

    if (target & TARGET_INDEX_CHANNEL) {
      c = m_cmap[c];
    }
    else {
      r = _rgba_getr(pal->getEntry(c));
      g = _rgba_getg(pal->getEntry(c));
      b = _rgba_getb(pal->getEntry(c));

      if (target & TARGET_RED_CHANNEL) r = m_cmap[r];
      if (target & TARGET_GREEN_CHANNEL) g = m_cmap[g];
      if (target & TARGET_BLUE_CHANNEL) b = m_cmap[b];

      c = rgbmap->mapColor(r, g, b);
    }

    *(dst_address++) = MID(0, c, pal->size()-1);
  }
}
