// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/color_curve_filter.h"

#include "filters/color_curve.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"
#include "doc/sprite.h"

#include <vector>

namespace filters {

using namespace doc;

ColorCurveFilter::ColorCurveFilter()
  : m_cmap(256)
{
}

void ColorCurveFilter::setCurve(const ColorCurve& curve)
{
  m_curve = curve;
  generateMap();
}

void ColorCurveFilter::generateMap()
{
  // Generate the color convertion map
  m_curve.getValues(0, 255, m_cmap);
  for (int c=0; c<256; c++)
    m_cmap[c] = std::clamp(m_cmap[c], 0, 255);
}

const char* ColorCurveFilter::getName()
{
  return "Color Curve";
}

void ColorCurveFilter::applyToRgba(FilterManager* filterMgr)
{
  int c, r, g, b, a;

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint32_t) {
    c = *src_address;

    r = rgba_getr(c);
    g = rgba_getg(c);
    b = rgba_getb(c);
    a = rgba_geta(c);

    if (target & TARGET_RED_CHANNEL) r = m_cmap[r];
    if (target & TARGET_GREEN_CHANNEL) g = m_cmap[g];
    if (target & TARGET_BLUE_CHANNEL) b = m_cmap[b];
    if (target & TARGET_ALPHA_CHANNEL) a = m_cmap[a];

    *dst_address = rgba(r, g, b, a);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void ColorCurveFilter::applyToGrayscale(FilterManager* filterMgr)
{
  int c, k, a;

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint16_t) {
    c = *src_address;

    k = graya_getv(c);
    a = graya_geta(c);

    if (target & TARGET_GRAY_CHANNEL) k = m_cmap[k];
    if (target & TARGET_ALPHA_CHANNEL) a = m_cmap[a];

    *dst_address = graya(k, a);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void ColorCurveFilter::applyToIndexed(FilterManager* filterMgr)
{
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
  int c, r, g, b, a;

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint8_t) {
    c = *src_address;

    if (target & TARGET_INDEX_CHANNEL) {
      c = m_cmap[c];
    }
    else {
      c = pal->getEntry(c);
      r = rgba_getr(c);
      g = rgba_getg(c);
      b = rgba_getb(c);
      a = rgba_geta(c);

      if (target & TARGET_RED_CHANNEL  ) r = m_cmap[r];
      if (target & TARGET_GREEN_CHANNEL) g = m_cmap[g];
      if (target & TARGET_BLUE_CHANNEL ) b = m_cmap[b];
      if (target & TARGET_ALPHA_CHANNEL) a = m_cmap[a];

      c = rgbmap->mapColor(r, g, b, a);
    }

    *dst_address = std::clamp(c, 0, pal->size()-1);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

} // namespace filters
