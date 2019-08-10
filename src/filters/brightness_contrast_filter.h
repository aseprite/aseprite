// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_BRIGHTNESS_CONTRAST_FILTER_H_INCLUDED
#define FILTERS_BRIGHTNESS_CONTRAST_FILTER_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/palette_picks.h"
#include "filters/filter.h"
#include "filters/target.h"

#include <vector>

namespace filters {

  class BrightnessContrastFilter : public FilterWithPalette {
  public:
    BrightnessContrastFilter();

    double brightness() const { return m_brightness; }
    double contrast() const { return m_contrast; }
    void setBrightness(double brightness);
    void setContrast(double contrast);

    // Filter implementation
    const char* getName() override;
    void applyToRgba(FilterManager* filterMgr) override;
    void applyToGrayscale(FilterManager* filterMgr) override;
    void applyToIndexed(FilterManager* filterMgr) override;

  private:
    void onApplyToPalette(FilterManager* filterMgr,
                          const doc::PalettePicks& picks) override;
    void applyFilterToRgb(const Target target, doc::color_t& color);
    void updateMap();

    double m_brightness, m_contrast;
    std::vector<int> m_cmap;
  };

} // namespace filters

#endif
