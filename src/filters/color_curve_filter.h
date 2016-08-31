// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_COLOR_CURVE_FILTER_H_INCLUDED
#define FILTERS_COLOR_CURVE_FILTER_H_INCLUDED
#pragma once

#include <vector>

#include "filters/filter.h"

namespace filters {

  class ColorCurve;

  class ColorCurveFilter : public Filter
  {
  public:
    ColorCurveFilter();

    void setCurve(ColorCurve* curve);
    ColorCurve* getCurve() const { return m_curve; }

    // Filter implementation
    const char* getName();
    void applyToRgba(FilterManager* filterMgr);
    void applyToGrayscale(FilterManager* filterMgr);
    void applyToIndexed(FilterManager* filterMgr);

  private:
    ColorCurve* m_curve;
    std::vector<int> m_cmap;
  };

} // namespace filters

#endif
