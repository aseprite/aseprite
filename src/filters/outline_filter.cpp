// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/outline_filter.h"

#include "doc/image.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "filters/neighboring_pixels.h"

#include <algorithm>

namespace filters {

using namespace doc;

namespace {

  struct GetPixelsDelegate {
    color_t bgColor;
    int transparent;    // Transparent pixels
    int opaque;         // Opaque pixels
    int matrix;
    int bit;

    void init(const color_t bgColor,
              const OutlineFilter::Matrix matrix) {
      this->bgColor = bgColor;
      this->matrix = (int)matrix;
    }

    void reset() {
      transparent = opaque = 0;
      bit = 1;
    }
  };

  struct GetPixelsDelegateRgba : public GetPixelsDelegate {
    void operator()(RgbTraits::pixel_t color) {
      if (rgba_geta(color) == 0 || color == bgColor)
        transparent += (matrix & bit ? 1: 0);
      else
        opaque += (matrix & bit ? 1: 0);
      bit <<= 1;
    }
  };

  struct GetPixelsDelegateGrayscale : public GetPixelsDelegate {
    void operator()(GrayscaleTraits::pixel_t color) {
      if (graya_geta(color) == 0 || color == bgColor)
        transparent += (matrix & bit ? 1: 0);
      else
        opaque += (matrix & bit ? 1: 0);
      bit <<= 1;
    }
  };

  struct GetPixelsDelegateIndexed : public GetPixelsDelegate {
    const Palette* pal;

    GetPixelsDelegateIndexed(const Palette* pal) : pal(pal) { }

    void operator()(IndexedTraits::pixel_t color) {
      color_t rgba = pal->getEntry(color);
      if (rgba_geta(rgba) == 0 || color == bgColor)
        transparent += (matrix & bit ? 1: 0);
      else
        opaque += (matrix & bit ? 1: 0);
      bit <<= 1;
    }
  };

}

OutlineFilter::OutlineFilter()
  : m_place(Place::Outside)
  , m_matrix(Matrix::Circle)
  , m_tiledMode(TiledMode::NONE)
  , m_color(0)
  , m_bgColor(0)
{
}

const char* OutlineFilter::getName()
{
  return "Outline";
}

void OutlineFilter::applyToRgba(FilterManager* filterMgr)
{
  const Image* src = filterMgr->getSourceImage();
  int r, g, b, a, n;
  color_t c;
  bool isTransparent;

  GetPixelsDelegateRgba delegate;
  delegate.init(m_bgColor, m_matrix);

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint32_t) {
    delegate.reset();
    get_neighboring_pixels<RgbTraits>(src, x, y, 3, 3, 1, 1, m_tiledMode, delegate);

    c = *src_address;
    n = (m_place == Place::Outside ? delegate.opaque: delegate.transparent);
    isTransparent = (rgba_geta(c) == 0 || c == m_bgColor);

    if ((n >= 1) &&
        ((m_place == Place::Outside && isTransparent) ||
         (m_place == Place::Inside && !isTransparent))) {
      r = (target & TARGET_RED_CHANNEL   ? rgba_getr(m_color): rgba_getr(c));
      g = (target & TARGET_GREEN_CHANNEL ? rgba_getg(m_color): rgba_getg(c));
      b = (target & TARGET_BLUE_CHANNEL  ? rgba_getb(m_color): rgba_getb(c));
      a = (target & TARGET_ALPHA_CHANNEL ? rgba_geta(m_color): rgba_geta(c));
      c = rgba(r, g, b, a);
    }

    *dst_address = c;
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void OutlineFilter::applyToGrayscale(FilterManager* filterMgr)
{
  const Image* src = filterMgr->getSourceImage();
  int k, a, n;
  color_t c;
  bool isTransparent;

  GetPixelsDelegateGrayscale delegate;
  delegate.init(m_bgColor, m_matrix);

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint16_t) {
    delegate.reset();
    get_neighboring_pixels<GrayscaleTraits>(src, x, y, 3, 3, 1, 1, m_tiledMode, delegate);

    c = *src_address;
    n = (m_place == Place::Outside ? delegate.opaque: delegate.transparent);
    isTransparent = (graya_geta(c) == 0 || c == m_bgColor);

    if ((n >= 1) &&
        ((m_place == Place::Outside && isTransparent) ||
         (m_place == Place::Inside && !isTransparent))) {
      k = (target & TARGET_GRAY_CHANNEL  ? graya_getv(m_color): graya_getv(c));
      a = (target & TARGET_ALPHA_CHANNEL ? graya_geta(m_color): graya_geta(c));
      c = graya(k, a);
    }

    *dst_address = c;
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void OutlineFilter::applyToIndexed(FilterManager* filterMgr)
{
  const Image* src = filterMgr->getSourceImage();
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
  int r, g, b, a, n;
  color_t c;
  bool isTransparent;

  GetPixelsDelegateIndexed delegate(pal);
  delegate.init(m_bgColor, m_matrix);

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint8_t) {
    delegate.reset();
    get_neighboring_pixels<IndexedTraits>(src, x, y, 3, 3, 1, 1, m_tiledMode, delegate);

    c = *src_address;
    n = (m_place == Place::Outside ? delegate.opaque: delegate.transparent);

    if (target & TARGET_INDEX_CHANNEL) {
      isTransparent = (c == m_bgColor);
    }
    else {
      isTransparent = (rgba_geta(pal->getEntry(c)) == 0 || c == m_bgColor);
    }

    if ((n >= 1) &&
        ((m_place == Place::Outside && isTransparent) ||
         (m_place == Place::Inside && !isTransparent))) {
      if (target & TARGET_INDEX_CHANNEL) {
        c = m_color;
      }
      else {
        c = pal->getEntry(c);
        r = (target & TARGET_RED_CHANNEL   ? rgba_getr(m_color): rgba_getr(c));
        g = (target & TARGET_GREEN_CHANNEL ? rgba_getg(m_color): rgba_getg(c));
        b = (target & TARGET_BLUE_CHANNEL  ? rgba_getb(m_color): rgba_getb(c));
        a = (target & TARGET_ALPHA_CHANNEL ? rgba_geta(m_color): rgba_geta(c));
        c = rgbmap->mapColor(r, g, b, a);
      }
    }

    *dst_address = c;
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

} // namespace filters
