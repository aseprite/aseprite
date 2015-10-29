// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_STROKE_H_INCLUDED
#define APP_TOOLS_STROKE_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"

#include <vector>

namespace app {
  namespace tools {

    class Stroke {
    public:
      typedef std::vector<gfx::Point> Points;
      typedef Points::const_iterator const_iterator;

      const_iterator begin() const { return m_points.begin(); }
      const_iterator end() const { return m_points.end(); }

      bool empty() const { return m_points.empty(); }
      int size() const { return (int)m_points.size(); }

      const gfx::Point& operator[](int i) const { return m_points[i]; }
      gfx::Point& operator[](int i) { return m_points[i]; }

      const gfx::Point& firstPoint() const {
        ASSERT(!m_points.empty());
        return m_points[0];
      }

      const gfx::Point& lastPoint() const {
        ASSERT(!m_points.empty());
        return m_points[m_points.size()-1];
      }

      // Clears the whole stroke.
      void reset();

      // Reset the stroke as "n" points in the given "point" position.
      void reset(int n, const gfx::Point& point);

      // Adds a new point to the stroke.
      void addPoint(const gfx::Point& point);

      // Displaces all X,Y coordinates the given delta.
      void offset(const gfx::Point& delta);

      // Returns the bounds of the stroke (minimum/maximum position).
      gfx::Rect bounds() const;

    public:
      Points m_points;
    };

    typedef std::vector<Stroke> Strokes;

  } // namespace tools
} // namespace app

#endif
