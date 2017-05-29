// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/hue_saturation_filter.h"

#include "doc/image.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "gfx/hsl.h"
#include "gfx/rgb.h"

#include <cmath>

namespace filters {

using namespace doc;

const char* HueSaturationFilter::getName()
{
  return "Hue Saturation Color";
}

HueSaturationFilter::HueSaturationFilter()
  : m_h(0.0)
  , m_s(0.0)
  , m_l(0.0)
  , m_a(0)
{
}

void HueSaturationFilter::setHue(double h)
{
  m_h = h;
}

void HueSaturationFilter::setSaturation(double s)
{
  m_s = s;
}

void HueSaturationFilter::setLightness(double l)
{
  m_l = l;
}

void HueSaturationFilter::setAlpha(int a)
{
  m_a = a;
}

void HueSaturationFilter::applyToRgba(FilterManager* filterMgr)
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

    r = rgba_getr(c);
    g = rgba_getg(c);
    b = rgba_getb(c);
    a = rgba_geta(c);

    {
      gfx::Hsl hsl(gfx::Rgb(r, g, b));

      double h = hsl.hue() + m_h;
      while (h < 0.0)
        h += 360.0;
      h = std::fmod(h, 360.0);

      double s = hsl.saturation() + m_s;
      s = MID(0.0, s, 1.0);

      double l = hsl.lightness() + m_l;
      l = MID(0.0, l, 1.0);

      hsl.hue(h);
      hsl.saturation(s);
      hsl.lightness(l);
      gfx::Rgb rgb(hsl);

      if (target & TARGET_RED_CHANNEL  ) r = rgb.red();
      if (target & TARGET_GREEN_CHANNEL) g = rgb.green();
      if (target & TARGET_BLUE_CHANNEL ) b = rgb.blue();

      if (a && (target & TARGET_ALPHA_CHANNEL))
        a = MID(0, a+m_a, 255);
    }

    *(dst_address++) = rgba(r, g, b, a);
  }
}

void HueSaturationFilter::applyToGrayscale(FilterManager* filterMgr)
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

    k = graya_getv(c);
    a = graya_geta(c);

    {
      gfx::Hsl hsl(gfx::Rgb(k, k, k));

      double l = hsl.lightness() + m_l;
      l = MID(0.0, l, 1.0);

      hsl.lightness(l);
      gfx::Rgb rgb(hsl);

      if (target & TARGET_GRAY_CHANNEL) k = rgb.red();

      if (a && (target & TARGET_ALPHA_CHANNEL))
        a = MID(0, a+m_a, 255);
    }

    *(dst_address++) = graya(k, a);
  }
}

void HueSaturationFilter::applyToIndexed(FilterManager* filterMgr)
{
  const uint8_t* src_address = (uint8_t*)filterMgr->getSourceAddress();
  uint8_t* dst_address = (uint8_t*)filterMgr->getDestinationAddress();
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
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
    c = pal->getEntry(c);
    r = rgba_getr(c);
    g = rgba_getg(c);
    b = rgba_getb(c);
    a = rgba_geta(c);

    {
      gfx::Hsl hsl(gfx::Rgb(r, g, b));

      double h = hsl.hue() + m_h;
      while (h < 0.0) h += 360.0;
      h = std::fmod(h, 360.0);

      double s = hsl.saturation() + m_s;
      s = MID(0.0, s, 1.0);

      double l = hsl.lightness() + m_l;
      l = MID(0.0, l, 1.0);

      hsl.hue(h);
      hsl.saturation(s);
      hsl.lightness(l);
      gfx::Rgb rgb(hsl);

      if (target & TARGET_RED_CHANNEL  ) r = rgb.red();
      if (target & TARGET_GREEN_CHANNEL) g = rgb.green();
      if (target & TARGET_BLUE_CHANNEL ) b = rgb.blue();

      if (a && (target & TARGET_ALPHA_CHANNEL))
        a = MID(0, a+m_a, 255);
    }

    c = rgbmap->mapColor(r, g, b, a);
    *(dst_address++) = c;
  }
}

} // namespace filters
