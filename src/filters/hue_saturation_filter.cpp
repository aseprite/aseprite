// Aseprite
// Copyright (C) 2017-2018  David Capello
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
#include "gfx/hsv.h"
#include "gfx/rgb.h"

#include <cmath>

namespace filters {

using namespace doc;

const char* HueSaturationFilter::getName()
{
  return "Hue Saturation Color";
}

HueSaturationFilter::HueSaturationFilter()
  : m_mode(Mode::HSL)
  , m_h(0.0)
  , m_s(0.0)
  , m_l(0.0)
  , m_a(0.0)
{
}

void HueSaturationFilter::setMode(Mode mode)
{
  m_mode = mode;
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

void HueSaturationFilter::setAlpha(double a)
{
  m_a = a;
}

void HueSaturationFilter::applyToRgba(FilterManager* filterMgr)
{
  FilterIndexedData* fid = filterMgr->getIndexedData();

  if (filterMgr->isFirstRow()) {
    m_picks = fid->getPalettePicks();
    m_usePalette = (m_picks.picks() > 0);
    if (m_usePalette)
      applyToPalette(filterMgr);
  }

  const Palette* pal = fid->getPalette();
  const uint32_t* src_address = (uint32_t*)filterMgr->getSourceAddress();
  uint32_t* dst_address = (uint32_t*)filterMgr->getDestinationAddress();
  const int w = filterMgr->getWidth();
  const Target target = filterMgr->getTarget();

  for (int x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    color_t c = *(src_address++);

    if (m_usePalette) {
      int i =
        pal->findExactMatch(rgba_getr(c),
                            rgba_getg(c),
                            rgba_getb(c),
                            rgba_geta(c), -1);
      if (i >= 0)
        c = fid->getNewPalette()->getEntry(i);
    }
    else {
      applyFilterToRgb(target, c);
    }

    *(dst_address++) = c;
  }
}

void HueSaturationFilter::applyToGrayscale(FilterManager* filterMgr)
{
  const uint16_t* src_address = (uint16_t*)filterMgr->getSourceAddress();
  uint16_t* dst_address = (uint16_t*)filterMgr->getDestinationAddress();
  const int w = filterMgr->getWidth();
  const Target target = filterMgr->getTarget();

  for (int x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    color_t c = *(src_address++);
    int k = graya_getv(c);
    int a = graya_geta(c);

    {
      gfx::Hsl hsl(gfx::Rgb(k, k, k));

      double l = hsl.lightness()*(1.0+m_l);
      l = MID(0.0, l, 1.0);

      hsl.lightness(l);
      gfx::Rgb rgb(hsl);

      if (target & TARGET_GRAY_CHANNEL) k = rgb.red();

      if (a && (target & TARGET_ALPHA_CHANNEL)) {
        a = a*(1.0+m_a);
        a = MID(0, a, 255);
      }
    }

    *(dst_address++) = graya(k, a);
  }
}

void HueSaturationFilter::applyToIndexed(FilterManager* filterMgr)
{
  FilterIndexedData* fid = filterMgr->getIndexedData();

  // Apply filter to color palette if there is no selection
  if (!filterMgr->isMaskActive()) {
    if (!filterMgr->isFirstRow())
      return;

    m_picks = fid->getPalettePicks();
    if (m_picks.picks() == 0)
      m_picks.all();

    applyToPalette(filterMgr);
    return;
  }

  // Apply filter to color region
  const Target target = filterMgr->getTarget();
  const Palette* pal = fid->getPalette();
  const RgbMap* rgbmap = fid->getRgbMap();
  const uint8_t* src_address = (uint8_t*)filterMgr->getSourceAddress();
  uint8_t* dst_address = (uint8_t*)filterMgr->getDestinationAddress();
  const int w = filterMgr->getWidth();

  for (int x=0; x<w; x++) {
    if (filterMgr->skipPixel()) {
      ++src_address;
      ++dst_address;
      continue;
    }

    color_t c = pal->getEntry(*(src_address++));
    applyFilterToRgb(target, c);
    *(dst_address++) = rgbmap->mapColor(rgba_getr(c),
                                        rgba_getg(c),
                                        rgba_getb(c),
                                        rgba_geta(c));
  }
}

void HueSaturationFilter::applyToPalette(FilterManager* filterMgr)
{
  const Target target = filterMgr->getTarget();
  FilterIndexedData* fid = filterMgr->getIndexedData();
  const Palette* pal = fid->getPalette();
  Palette* newPal = fid->getNewPalette();

  int i = 0;
  for (bool state : m_picks) {
    if (!state) {
      ++i;
      continue;
    }

    color_t c = pal->getEntry(i);
    applyFilterToRgb(target, c);
    newPal->setEntry(i, c);
    ++i;
  }
}

template<class T,
         double (T::*get_lightness)() const,
         void (T::*set_lightness)(double)>
void HueSaturationFilter::applyFilterToRgbT(const Target target, doc::color_t& c)
{
  int r = rgba_getr(c);
  int g = rgba_getg(c);
  int b = rgba_getb(c);
  int a = rgba_geta(c);

  T hsl(gfx::Rgb(r, g, b));

  double h = hsl.hue() + m_h;
  while (h < 0.0) h += 360.0;
  h = std::fmod(h, 360.0);

  double s = hsl.saturation()*(1.0+m_s);
  s = MID(0.0, s, 1.0);

  double l = (hsl.*get_lightness)()*(1.0+m_l);
  l = MID(0.0, l, 1.0);

  hsl.hue(h);
  hsl.saturation(s);
  (hsl.*set_lightness)(l);
  gfx::Rgb rgb(hsl);

  if (target & TARGET_RED_CHANNEL  ) r = rgb.red();
  if (target & TARGET_GREEN_CHANNEL) g = rgb.green();
  if (target & TARGET_BLUE_CHANNEL ) b = rgb.blue();
  if (a && (target & TARGET_ALPHA_CHANNEL)) {
    a = a*(1.0+m_a);
    a = MID(0, a, 255);
  }

  c = rgba(r, g, b, a);
}

void HueSaturationFilter::applyFilterToRgb(const Target target, doc::color_t& color)
{
  switch (m_mode) {
    case Mode::HSL:
      applyFilterToRgbT<gfx::Hsl,
                        &gfx::Hsl::lightness,
                        &gfx::Hsl::lightness>(target, color);
      break;
    case Mode::HSV:
      applyFilterToRgbT<gfx::Hsv,
                        &gfx::Hsv::value,
                        &gfx::Hsv::value>(target, color);
      break;
  }
}

} // namespace filters
