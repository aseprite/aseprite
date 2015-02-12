// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef FILTERS_COLOR_CURVE_H_INCLUDED
#define FILTERS_COLOR_CURVE_H_INCLUDED
#pragma once

#include <vector>

#include "gfx/point.h"

namespace filters {

  class ColorCurve {
  public:
    enum Type {
      Linear,
      //Spline,                      // TODO for the future
    };

    typedef std::vector<gfx::Point> Points;

    typedef Points::iterator iterator;
    typedef Points::const_iterator const_iterator;

    ColorCurve(Type type);

    iterator begin() { return m_points.begin(); }
    iterator end() { return m_points.end(); }

    const_iterator begin() const { return m_points.begin(); }
    const_iterator end() const { return m_points.end(); }

    void addPoint(const gfx::Point& point);
    void removePoint(const gfx::Point& point);

    void getValues(int x1, int x2, std::vector<int>& values);

  private:
    Type m_type;
    Points m_points;
  };

} // namespace filters

#endif
