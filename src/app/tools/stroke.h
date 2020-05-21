// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
      struct Pt {
        int x = 0;
        int y = 0;
        float size = 0.0f;
        float angle = 0.0f;
        float gradient = 0.0f;
        Pt() { }
        Pt(const gfx::Point& point) : x(point.x), y(point.y) { }
        Pt(int x, int y) : x(x), y(y) { }
        gfx::Point toPoint() const { return gfx::Point(x, y); }
        bool operator==(const Pt& that) const { return x == that.x && y == that.y; }
        bool operator!=(const Pt& that) const { return x != that.x || y != that.y; }
      };
      typedef std::vector<Pt> Pts;
      typedef Pts::const_iterator const_iterator;

      const_iterator begin() const { return m_pts.begin(); }
      const_iterator end() const { return m_pts.end(); }

      bool empty() const { return m_pts.empty(); }
      int size() const { return (int)m_pts.size(); }

      const Pt& operator[](int i) const { return m_pts[i]; }
      Pt& operator[](int i) { return m_pts[i]; }

      const Pt& firstPoint() const {
        ASSERT(!m_pts.empty());
        return m_pts[0];
      }

      const Pt& lastPoint() const {
        ASSERT(!m_pts.empty());
        return m_pts[m_pts.size()-1];
      }

      // Clears the whole stroke.
      void reset();

      // Reset the stroke as "n" points in the given point position.
      void reset(int n, const Pt& pt);

      // Adds a new point to the stroke.
      void addPoint(const Pt& pt);

      // Displaces all X,Y coordinates the given delta.
      void offset(const gfx::Point& delta);

      // Erase the point "index".
      void erase(int index);

      // Returns the bounds of the stroke (minimum/maximum position).
      gfx::Rect bounds() const;

      std::vector<int> toXYInts() const;

    public:
      Pts m_pts;
    };

    typedef std::vector<Stroke> Strokes;

  } // namespace tools
} // namespace app

#endif
