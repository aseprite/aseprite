// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
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
#include "doc/palette_picks.h"
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
  : m_mode(Mode::HSL_MUL)
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
  const Palette* pal = fid->getPalette();
  Palette* newPal = (m_usePaletteOnRGB ? fid->getNewPalette(): nullptr);

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint32_t) {
    color_t c = *src_address;

    if (newPal) {
      int i =
        pal->findExactMatch(rgba_getr(c),
                            rgba_getg(c),
                            rgba_getb(c),
                            rgba_geta(c), -1);
      if (i >= 0)
        c = newPal->getEntry(i);
    }
    else {
      applyFilterToRgb(target, c);
    }

    *dst_address = c;
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void HueSaturationFilter::applyToGrayscale(FilterManager* filterMgr)
{
  FILTER_LOOP_THROUGH_ROW_BEGIN(uint16_t) {
    color_t c = *src_address;
    int k = graya_getv(c);
    int a = graya_geta(c);

    {
      gfx::Hsl hsl(gfx::Rgb(k, k, k));

      double l = hsl.lightness()*(1.0+m_l);
      l = std::clamp(l, 0.0, 1.0);

      hsl.lightness(l);
      gfx::Rgb rgb(hsl);

      if (target & TARGET_GRAY_CHANNEL) k = rgb.red();

      if (a && (target & TARGET_ALPHA_CHANNEL)) {
        a = a*(1.0+m_a);
        a = std::clamp(a, 0, 255);
      }
    }

    *dst_address = graya(k, a);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void HueSaturationFilter::applyToIndexed(FilterManager* filterMgr)
{
  // Apply filter to pixels if there is selection (in other case, the
  // change is global, so we have already applied the filter to the
  // palette).
  if (!filterMgr->isMaskActive())
    return;

  // Apply filter to color region
  FilterIndexedData* fid = filterMgr->getIndexedData();
  const Palette* pal = fid->getPalette();
  const RgbMap* rgbmap = fid->getRgbMap();

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint8_t) {
    color_t c = pal->getEntry(*src_address);
    applyFilterToRgb(target, c);
    *dst_address = rgbmap->mapColor(c);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void HueSaturationFilter::onApplyToPalette(FilterManager* filterMgr,
                                           const PalettePicks& picks)
{
  FilterIndexedData* fid = filterMgr->getIndexedData();
  const Target target = filterMgr->getTarget();
  const Palette* pal = fid->getPalette();
  Palette* newPal = fid->getNewPalette();

  int i = 0;
  for (bool state : picks) {
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
void HueSaturationFilter::applyFilterToRgbT(const Target target,
                                            doc::color_t& c,
                                            bool multiply)
{
  int r = rgba_getr(c);
  int g = rgba_getg(c);
  int b = rgba_getb(c);
  int a = rgba_geta(c);

  T hsl(gfx::Rgb(r, g, b));

  double h = hsl.hue() + m_h;
  while (h < 0.0) h += 360.0;
  h = std::fmod(h, 360.0);

  double s = (multiply ? hsl.saturation()*(1.0+m_s):
                         hsl.saturation() + m_s);
  s = std::clamp(s, 0.0, 1.0);

  double l = (multiply ? (hsl.*get_lightness)()*(1.0+m_l):
                         (hsl.*get_lightness)() + m_l);
  l = std::clamp(l, 0.0, 1.0);

  hsl.hue(h);
  hsl.saturation(s);
  (hsl.*set_lightness)(l);
  gfx::Rgb rgb(hsl);

  if (target & TARGET_RED_CHANNEL  ) r = rgb.red();
  if (target & TARGET_GREEN_CHANNEL) g = rgb.green();
  if (target & TARGET_BLUE_CHANNEL ) b = rgb.blue();
  if (a && (target & TARGET_ALPHA_CHANNEL)) {
    a = a*(1.0+m_a);
    a = std::clamp(a, 0, 255);
  }

  c = rgba(r, g, b, a);
}

void HueSaturationFilter::applyFilterToRgb(const Target target, doc::color_t& color)
{
  switch (m_mode) {
    case Mode::HSV_MUL:
      applyFilterToRgbT<gfx::Hsv,
                        &gfx::Hsv::value,
                        &gfx::Hsv::value>(target, color, true);
      break;
    case Mode::HSL_MUL:
      applyFilterToRgbT<gfx::Hsl,
                        &gfx::Hsl::lightness,
                        &gfx::Hsl::lightness>(target, color, true);
      break;
    case Mode::HSV_ADD:
      applyFilterToRgbT<gfx::Hsv,
                        &gfx::Hsv::value,
                        &gfx::Hsv::value>(target, color, false);
      break;
    case Mode::HSL_ADD:
      applyFilterToRgbT<gfx::Hsl,
                        &gfx::Hsl::lightness,
                        &gfx::Hsl::lightness>(target, color, false);
      break;
  }
}

} // namespace filters
