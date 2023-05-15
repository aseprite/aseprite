// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/invert_color_filter.h"

#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"

namespace filters {

using namespace doc;

const char* InvertColorFilter::getName()
{
  return "Invert Color";
}

void InvertColorFilter::applyToRgba(FilterManager* filterMgr)
{
  int c, r, g, b, a;

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint32_t) {
    c = *src_address;

    r = rgba_getr(c);
    g = rgba_getg(c);
    b = rgba_getb(c);
    a = rgba_geta(c);

    if (target & TARGET_RED_CHANNEL) r ^= 0xff;
    if (target & TARGET_GREEN_CHANNEL) g ^= 0xff;
    if (target & TARGET_BLUE_CHANNEL) b ^= 0xff;
    if (target & TARGET_ALPHA_CHANNEL) a ^= 0xff;

    *dst_address = rgba(r, g, b, a);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void InvertColorFilter::applyToGrayscale(FilterManager* filterMgr)
{
  int c, k, a;

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint16_t) {
    c = *src_address;

    k = graya_getv(c);
    a = graya_geta(c);

    if (target & TARGET_GRAY_CHANNEL) k ^= 0xff;
    if (target & TARGET_ALPHA_CHANNEL) a ^= 0xff;

    *dst_address = graya(k, a);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void InvertColorFilter::applyToIndexed(FilterManager* filterMgr)
{
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
  int c, r, g, b, a;

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint8_t) {
    c = *src_address;

    if (target & TARGET_INDEX_CHANNEL)
      c ^= 0xff;
    else {
      c = pal->getEntry(c);
      r = rgba_getr(c);
      g = rgba_getg(c);
      b = rgba_getb(c);
      a = rgba_geta(c);

      if (target & TARGET_RED_CHANNEL  ) r ^= 0xff;
      if (target & TARGET_GREEN_CHANNEL) g ^= 0xff;
      if (target & TARGET_BLUE_CHANNEL ) b ^= 0xff;
      if (target & TARGET_ALPHA_CHANNEL) a ^= 0xff;

      c = rgbmap->mapColor(r, g, b, a);
    }

    *dst_address = c;
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

} // namespace filters
