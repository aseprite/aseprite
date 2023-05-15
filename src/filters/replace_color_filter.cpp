// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/replace_color_filter.h"

#include "doc/image.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"

namespace filters {

using namespace doc;

ReplaceColorFilter::ReplaceColorFilter()
{
  m_from = m_to = 0;
  m_tolerance = 0;
}

void ReplaceColorFilter::setFrom(const color_t from)
{
  m_from = from;
}

void ReplaceColorFilter::setTo(const color_t to)
{
  m_to = to;
}

void ReplaceColorFilter::setTolerance(int tolerance)
{
  m_tolerance = std::clamp(tolerance, 0, 255);
}

const char* ReplaceColorFilter::getName()
{
  return "Replace Color";
}

void ReplaceColorFilter::applyToRgba(FilterManager* filterMgr)
{
  int from_r, from_g, from_b, from_a;
  int src_r, src_g, src_b, src_a;
  int to_r, to_g, to_b, to_a;
  int c;

  from_r = rgba_getr(m_from);
  from_g = rgba_getg(m_from);
  from_b = rgba_getb(m_from);
  from_a = rgba_geta(m_from);
  to_r = rgba_getr(m_to);
  to_g = rgba_getg(m_to);
  to_b = rgba_getb(m_to);
  to_a = rgba_geta(m_to);

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint32_t) {
    c = *src_address;
    src_r = rgba_getr(c);
    src_g = rgba_getg(c);
    src_b = rgba_getb(c);
    src_a = rgba_geta(c);

    if (!(target & TARGET_RED_CHANNEL  )) from_r = src_r;
    if (!(target & TARGET_GREEN_CHANNEL)) from_g = src_g;
    if (!(target & TARGET_BLUE_CHANNEL )) from_b = src_b;
    if (!(target & TARGET_ALPHA_CHANNEL)) from_a = src_a;

    if ((ABS(src_r-from_r) <= m_tolerance) &&
        (ABS(src_g-from_g) <= m_tolerance) &&
        (ABS(src_b-from_b) <= m_tolerance) &&
        (ABS(src_a-from_a) <= m_tolerance)) {
      *dst_address = rgba(
        (target & TARGET_RED_CHANNEL   ? to_r: src_r),
        (target & TARGET_GREEN_CHANNEL ? to_g: src_g),
        (target & TARGET_BLUE_CHANNEL  ? to_b: src_b),
        (target & TARGET_ALPHA_CHANNEL ? to_a: src_a));
    }
    else
      *dst_address = c;
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void ReplaceColorFilter::applyToGrayscale(FilterManager* filterMgr)
{
  int from_v, from_a;
  int src_v, src_a;
  int to_v, to_a;
  int c;

  from_v = graya_getv(m_from);
  from_a = graya_geta(m_from);
  to_v = graya_getv(m_to);
  to_a = graya_geta(m_to);

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint16_t) {
    c = *src_address;
    src_v = graya_getv(c);
    src_a = graya_geta(c);

    if (!(target & TARGET_GRAY_CHANNEL )) from_v = src_v;
    if (!(target & TARGET_ALPHA_CHANNEL)) from_a = src_a;

    if ((ABS(src_v-from_v) <= m_tolerance) &&
        (ABS(src_a-from_a) <= m_tolerance)) {
      *dst_address = graya(
        (target & TARGET_GRAY_CHANNEL  ? to_v: src_v),
        (target & TARGET_ALPHA_CHANNEL ? to_a: src_a));
    }
    else
      *dst_address = c;
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void ReplaceColorFilter::applyToIndexed(FilterManager* filterMgr)
{
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
  int from_r, from_g, from_b, from_a;
  int src_r, src_g, src_b, src_a;
  int to_r, to_g, to_b, to_a;
  int c;

  c = pal->getEntry(m_from);
  from_r = rgba_getr(c);
  from_g = rgba_getg(c);
  from_b = rgba_getb(c);
  from_a = rgba_geta(c);

  c = pal->getEntry(m_to);
  to_r = rgba_getr(c);
  to_g = rgba_getg(c);
  to_b = rgba_getb(c);
  to_a = rgba_geta(c);

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint8_t) {
    c = *src_address;

    if (target & TARGET_INDEX_CHANNEL) {
      if (ABS(c-m_from) <= m_tolerance)
        *dst_address = m_to;
      else
        *dst_address = c;
    }
    else {
      src_r = rgba_getr(pal->getEntry(c));
      src_g = rgba_getg(pal->getEntry(c));
      src_b = rgba_getb(pal->getEntry(c));
      src_a = rgba_geta(pal->getEntry(c));

      if (!(target & TARGET_RED_CHANNEL  )) from_r = src_r;
      if (!(target & TARGET_GREEN_CHANNEL)) from_g = src_g;
      if (!(target & TARGET_BLUE_CHANNEL )) from_b = src_b;
      if (!(target & TARGET_ALPHA_CHANNEL)) from_a = src_a;

      if ((ABS(src_r-from_r) <= m_tolerance) &&
          (ABS(src_g-from_g) <= m_tolerance) &&
          (ABS(src_b-from_b) <= m_tolerance) &&
          (ABS(src_a-from_a) <= m_tolerance)) {
        *dst_address = rgbmap->mapColor(
          (target & TARGET_RED_CHANNEL   ? to_r: src_r),
          (target & TARGET_GREEN_CHANNEL ? to_g: src_g),
          (target & TARGET_BLUE_CHANNEL  ? to_b: src_b),
          (target & TARGET_ALPHA_CHANNEL ? to_a: src_a));
      }
      else
        *dst_address = c;
    }
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

} // namespace filters
