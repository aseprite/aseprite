// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/median_filter.h"

#include "base/memory.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "filters/neighboring_pixels.h"
#include "filters/tiled_mode.h"

#include <algorithm>

namespace filters {

using namespace doc;

namespace {
  struct GetPixelsDelegateRgba {
    std::vector<std::vector<uint8_t> >& channel;
    int c;

    GetPixelsDelegateRgba(std::vector<std::vector<uint8_t> >& channel) : channel(channel) { }

    void reset() { c = 0; }

    void operator()(RgbTraits::pixel_t color)
    {
      channel[0][c] = rgba_getr(color);
      channel[1][c] = rgba_getg(color);
      channel[2][c] = rgba_getb(color);
      channel[3][c] = rgba_geta(color);
      c++;
    }
  };

  struct GetPixelsDelegateGrayscale {
    std::vector<std::vector<uint8_t> >& channel;
    int c;

    GetPixelsDelegateGrayscale(std::vector<std::vector<uint8_t> >& channel) : channel(channel) { }

    void reset() { c = 0; }

    void operator()(GrayscaleTraits::pixel_t color)
    {
      channel[0][c] = graya_getv(color);
      channel[1][c] = graya_geta(color);
      c++;
    }
  };

  struct GetPixelsDelegateIndexed {
    const Palette* pal;
    std::vector<std::vector<uint8_t> >& channel;
    Target target;
    int c;

    GetPixelsDelegateIndexed(const Palette* pal, std::vector<std::vector<uint8_t> >& channel, Target target)
      : pal(pal), channel(channel), target(target) { }

    void reset() { c = 0; }

    void operator()(IndexedTraits::pixel_t color)
    {
      if (target & TARGET_INDEX_CHANNEL) {
        channel[0][c] = color;
      }
      else {
        color_t rgb = pal->getEntry(color);
        channel[0][c] = rgba_getr(rgb);
        channel[1][c] = rgba_getg(rgb);
        channel[2][c] = rgba_getb(rgb);
        channel[3][c] = rgba_geta(rgb);
      }
      c++;
    }
  };
};

MedianFilter::MedianFilter()
  : m_tiledMode(TiledMode::NONE)
  , m_width(1)
  , m_height(1)
  , m_ncolors(0)
  , m_channel(4)
{
}

void MedianFilter::setTiledMode(TiledMode tiled)
{
  m_tiledMode = tiled;
}

void MedianFilter::setSize(int width, int height)
{
  ASSERT(width >= 1);
  ASSERT(height >= 1);

  m_width = std::max(1, width);
  m_height = std::max(1, height);
  m_ncolors = width*height;

  for (int c = 0; c < 4; ++c)
    m_channel[c].resize(m_ncolors);
}

const char* MedianFilter::getName()
{
  return "Median Blur";
}

void MedianFilter::applyToRgba(FilterManager* filterMgr)
{
  const Image* src = filterMgr->getSourceImage();
  int color, r, g, b, a;
  GetPixelsDelegateRgba delegate(m_channel);

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint32_t) {
    delegate.reset();
    get_neighboring_pixels<RgbTraits>(src, x, y, m_width, m_height, m_width/2, m_height/2,
                                      m_tiledMode, delegate);

    color = get_pixel_fast<RgbTraits>(src, x, y);

    if (target & TARGET_RED_CHANNEL) {
      std::sort(m_channel[0].begin(), m_channel[0].end());
      r = m_channel[0][m_ncolors/2];
    }
    else
      r = rgba_getr(color);

    if (target & TARGET_GREEN_CHANNEL) {
      std::sort(m_channel[1].begin(), m_channel[1].end());
      g = m_channel[1][m_ncolors/2];
    }
    else
      g = rgba_getg(color);

    if (target & TARGET_BLUE_CHANNEL) {
      std::sort(m_channel[2].begin(), m_channel[2].end());
      b = m_channel[2][m_ncolors/2];
    }
    else
      b = rgba_getb(color);

    if (target & TARGET_ALPHA_CHANNEL) {
      std::sort(m_channel[3].begin(), m_channel[3].end());
      a = m_channel[3][m_ncolors/2];
    }
    else
      a = rgba_geta(color);

    *dst_address = rgba(r, g, b, a);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void MedianFilter::applyToGrayscale(FilterManager* filterMgr)
{
  const Image* src = filterMgr->getSourceImage();
  int color, k, a;
  GetPixelsDelegateGrayscale delegate(m_channel);

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint16_t) {
    delegate.reset();
    get_neighboring_pixels<GrayscaleTraits>(src, x, y, m_width, m_height, m_width/2, m_height/2,
                                            m_tiledMode, delegate);

    color = get_pixel_fast<GrayscaleTraits>(src, x, y);

    if (target & TARGET_GRAY_CHANNEL) {
      std::sort(m_channel[0].begin(), m_channel[0].end());
      k = m_channel[0][m_ncolors/2];
    }
    else
      k = graya_getv(color);

    if (target & TARGET_ALPHA_CHANNEL) {
      std::sort(m_channel[1].begin(), m_channel[1].end());
      a = m_channel[1][m_ncolors/2];
    }
    else
      a = graya_geta(color);

    *dst_address = graya(k, a);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void MedianFilter::applyToIndexed(FilterManager* filterMgr)
{
  const Image* src = filterMgr->getSourceImage();
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
  int color, r, g, b, a;
  GetPixelsDelegateIndexed delegate(pal, m_channel, filterMgr->getTarget());

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint8_t) {
    delegate.reset();
    get_neighboring_pixels<IndexedTraits>(src, x, y, m_width, m_height, m_width/2, m_height/2,
                                          m_tiledMode, delegate);

    if (target & TARGET_INDEX_CHANNEL) {
      std::sort(m_channel[0].begin(), m_channel[0].end());
      *dst_address = m_channel[0][m_ncolors/2];
    }
    else {
      color = get_pixel_fast<IndexedTraits>(src, x, y);
      color = pal->getEntry(color);

      if (target & TARGET_RED_CHANNEL) {
        std::sort(m_channel[0].begin(), m_channel[0].end());
        r = m_channel[0][m_ncolors/2];
      }
      else
        r = rgba_getr(color);

      if (target & TARGET_GREEN_CHANNEL) {
        std::sort(m_channel[1].begin(), m_channel[1].end());
        g = m_channel[1][m_ncolors/2];
      }
      else
        g = rgba_getg(pal->getEntry(color));

      if (target & TARGET_BLUE_CHANNEL) {
        std::sort(m_channel[2].begin(), m_channel[2].end());
        b = m_channel[2][m_ncolors/2];
      }
      else
        b = rgba_getb(color);

      if (target & TARGET_ALPHA_CHANNEL) {
        std::sort(m_channel[3].begin(), m_channel[3].end());
        a = m_channel[3][m_ncolors/2];
      }
      else
        a = rgba_geta(color);

      *dst_address = rgbmap->mapColor(r, g, b, a);
    }
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

} // namespace filters
