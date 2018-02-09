// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_HUE_SATURATION_FILTER_H_INCLUDED
#define FILTERS_HUE_SATURATION_FILTER_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/palette_picks.h"
#include "filters/filter.h"
#include "filters/target.h"

namespace filters {

  class HueSaturationFilter : public Filter {
  public:
    enum class Mode { HSL, HSV };

    HueSaturationFilter();

    void setMode(Mode mode);
    void setHue(double h);
    void setSaturation(double s);
    void setLightness(double v);
    void setAlpha(double a);

    // Filter implementation
    const char* getName();
    void applyToRgba(FilterManager* filterMgr);
    void applyToGrayscale(FilterManager* filterMgr);
    void applyToIndexed(FilterManager* filterMgr);

  private:
    void applyToPalette(FilterManager* filterMgr);
    template<class T,
             double (T::*get_lightness)() const,
             void (T::*set_lightness)(double)>
    void applyFilterToRgbT(const Target target, doc::color_t& color);
    void applyFilterToRgb(const Target target, doc::color_t& color);

    Mode m_mode;
    double m_h, m_s, m_l, m_a;
    doc::PalettePicks m_picks;
    bool m_usePalette;
  };

} // namespace filters

#endif
