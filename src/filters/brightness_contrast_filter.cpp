// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/brightness_contrast_filter.h"

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

const char* BrightnessContrastFilter::getName()
{
  return "Brightness Contrast";
}

BrightnessContrastFilter::BrightnessContrastFilter()
  : m_brightness(0.0)
  , m_contrast(0.0)
  , m_cmap(256)
{
  updateMap();
}

void BrightnessContrastFilter::setBrightness(double brightness)
{
  m_brightness = brightness;
  updateMap();
}

void BrightnessContrastFilter::setContrast(double contrast)
{
  m_contrast = contrast;
  updateMap();
}

void BrightnessContrastFilter::applyToRgba(FilterManager* filterMgr)
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

void BrightnessContrastFilter::applyToGrayscale(FilterManager* filterMgr)
{
  FILTER_LOOP_THROUGH_ROW_BEGIN(uint16_t) {
    color_t c = *src_address;
    int k = graya_getv(c);
    int a = graya_geta(c);

    if (target & TARGET_GRAY_CHANNEL) k = m_cmap[k];

    *dst_address = graya(k, a);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void BrightnessContrastFilter::applyToIndexed(FilterManager* filterMgr)
{
  FilterIndexedData* fid = filterMgr->getIndexedData();

  // Apply filter to pixels if there is selection (in other case, the
  // change is global, so we have already applied the filter to the
  // palette).
  if (!filterMgr->isMaskActive())
    return;

  // Apply filter to color region
  const Palette* pal = fid->getPalette();
  const RgbMap* rgbmap = fid->getRgbMap();

  FILTER_LOOP_THROUGH_ROW_BEGIN(uint8_t) {
    color_t c = pal->getEntry(*src_address);
    applyFilterToRgb(target, c);
    *dst_address = rgbmap->mapColor(c);
  }
  FILTER_LOOP_THROUGH_ROW_END()
}

void BrightnessContrastFilter::onApplyToPalette(FilterManager* filterMgr,
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

void BrightnessContrastFilter::applyFilterToRgb(
  const Target target, doc::color_t& c)
{
  int r = rgba_getr(c);
  int g = rgba_getg(c);
  int b = rgba_getb(c);
  int a = rgba_geta(c);

  if (target & TARGET_RED_CHANNEL  ) r = m_cmap[r];
  if (target & TARGET_GREEN_CHANNEL) g = m_cmap[g];
  if (target & TARGET_BLUE_CHANNEL ) b = m_cmap[b];

  c = rgba(r, g, b, a);
}

void BrightnessContrastFilter::updateMap()
{
  int max = int(m_cmap.size());
  for (int u=0; u<max; ++u) {
    double x = double(u) / double(max-1);
    double y = (m_contrast+1.0) * (x - 0.5) + 0.5;
    y = y*(1.0+m_brightness);
    y = std::clamp(y, 0.0, 1.0);
    m_cmap[u] = int(255.5 * y);
  }
}

} // namespace filters
