// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_COLOR_CURVE_FILTER_H_INCLUDED
#define FILTERS_COLOR_CURVE_FILTER_H_INCLUDED
#pragma once

#include <vector>

#include "filters/filter.h"
#include "filters/color_curve.h"

namespace filters {

  class ColorCurveFilter : public Filter {
  public:
    ColorCurveFilter();

    void setCurve(const ColorCurve& curve);
    const ColorCurve& getCurve() const { return m_curve; }

    // Filter implementation
    const char* getName();
    void applyToRgba(FilterManager* filterMgr);
    void applyToGrayscale(FilterManager* filterMgr);
    void applyToIndexed(FilterManager* filterMgr);

  private:
    void generateMap();

    ColorCurve m_curve;
    std::vector<int> m_cmap;
  };

} // namespace filters

#endif
