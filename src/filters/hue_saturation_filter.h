// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_HUE_SATURATION_FILTER_H_INCLUDED
#define FILTERS_HUE_SATURATION_FILTER_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "filters/filter.h"
#include "filters/target.h"

namespace filters {

  class HueSaturationFilter : public FilterWithPalette {
  public:
    enum class Mode {
      HSV_MUL, HSL_MUL,
      HSV_ADD, HSL_ADD,
    };

    HueSaturationFilter();

    void setMode(Mode mode);
    void setHue(double h);
    void setSaturation(double s);
    void setLightness(double v);
    void setAlpha(double a);

    // Filter implementation
    const char* getName() override;
    void applyToRgba(FilterManager* filterMgr) override;
    void applyToGrayscale(FilterManager* filterMgr) override;
    void applyToIndexed(FilterManager* filterMgr) override;

  private:
    void onApplyToPalette(FilterManager* filterMgr,
                          const doc::PalettePicks& picks) override;

    template<class T,
             double (T::*get_lightness)() const,
             void (T::*set_lightness)(double)>
    void applyFilterToRgbT(const Target target, doc::color_t& color, bool multiply);
    void applyFilterToRgb(const Target target, doc::color_t& color);

    Mode m_mode;
    double m_h, m_s, m_l, m_a;
  };

} // namespace filters

#endif
