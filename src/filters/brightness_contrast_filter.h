// Aseprite
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

  class BrightnessContrastFilter : public Filter {
  public:
    BrightnessContrastFilter();

    void setBrightness(double brightness);
    void setContrast(double contrast);

    // Filter implementation
    const char* getName();
    void applyToRgba(FilterManager* filterMgr);
    void applyToGrayscale(FilterManager* filterMgr);
    void applyToIndexed(FilterManager* filterMgr);

  private:
    void applyToPalette(FilterManager* filterMgr);
    void applyFilterToRgb(const Target target, doc::color_t& color);
    void updateMap();

    double m_brightness, m_contrast;
    doc::PalettePicks m_picks;
    bool m_usePalette;
    std::vector<int> m_cmap;
  };

} // namespace filters

#endif
