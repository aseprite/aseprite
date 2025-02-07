// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TRANSFORMATION_H_INCLUDED
#define APP_TRANSFORMATION_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"
#include <vector>

#define CORNER_THICK_FOR_TILEMAP_MODE 0.001
#define CORNER_THICK_FOR_PIXELS_MODE  1.0

namespace app {

// Represents a transformation that can be done by the user in the
// document when he/she moves the mask using the selection handles.
class Transformation {
public:
  class Corners {
  public:
    enum { LEFT_TOP = 0, RIGHT_TOP = 1, RIGHT_BOTTOM = 2, LEFT_BOTTOM = 3, NUM_OF_CORNERS = 4 };

    Corners() : m_corners(NUM_OF_CORNERS) {}
    Corners(const gfx::RectF bounds) : m_corners(NUM_OF_CORNERS)
    {
      m_corners[LEFT_TOP].x = bounds.x;
      m_corners[LEFT_TOP].y = bounds.y;
      m_corners[RIGHT_TOP].x = bounds.x2();
      m_corners[RIGHT_TOP].y = bounds.y;
      m_corners[RIGHT_BOTTOM].x = bounds.x2();
      m_corners[RIGHT_BOTTOM].y = bounds.y2();
      m_corners[LEFT_BOTTOM].x = bounds.x;
      m_corners[LEFT_BOTTOM].y = bounds.y2();
    }

    std::size_t size() const { return m_corners.size(); }

    gfx::PointF& operator[](int index) { return m_corners[index]; }
    const gfx::PointF& operator[](int index) const { return m_corners[index]; }

    const gfx::PointF& leftTop() const { return m_corners[LEFT_TOP]; }
    const gfx::PointF& rightTop() const { return m_corners[RIGHT_TOP]; }
    const gfx::PointF& rightBottom() const { return m_corners[RIGHT_BOTTOM]; }
    const gfx::PointF& leftBottom() const { return m_corners[LEFT_BOTTOM]; }

    void leftTop(const gfx::PointF& pt) { m_corners[LEFT_TOP] = pt; }
    void rightTop(const gfx::PointF& pt) { m_corners[RIGHT_TOP] = pt; }
    void rightBottom(const gfx::PointF& pt) { m_corners[RIGHT_BOTTOM] = pt; }
    void leftBottom(const gfx::PointF& pt) { m_corners[LEFT_BOTTOM] = pt; }

    gfx::RectF bounds(double cornerThick) const
    {
      gfx::RectF bounds;
      for (int i = 0; i < Corners::NUM_OF_CORNERS; ++i)
        bounds |= gfx::RectF(m_corners[i].x, m_corners[i].y, cornerThick, cornerThick);
      return bounds.floor();
    }

  private:
    std::vector<gfx::PointF> m_corners;
  };

  Transformation();
  Transformation(const gfx::RectF& bounds, double cornerThick);

  // Simple getters and setters. The angle is in radians.

  const gfx::RectF& bounds() const { return m_bounds; }
  const gfx::PointF& pivot() const { return m_pivot; }
  double cornerThick() const { return m_cornerThick; }
  double angle() const { return m_angle; }
  double skew() const { return m_skew; }

  void bounds(const gfx::RectF& bounds) { m_bounds = bounds; }
  void pivot(const gfx::PointF& pivot) { m_pivot = pivot; }
  void angle(double angle) { m_angle = angle; }
  void skew(double angle) { m_skew = angle; }

  // Applies the transformation (rotation with angle/pivot) to the
  // current bounds (m_bounds).
  Corners transformedCorners() const;

  // Changes the pivot to another location, adjusting the bounds to
  // keep the current rotated-corners in the same location.
  void displacePivotTo(const gfx::PointF& newPivot);

  gfx::RectF transformedBounds() const;

  // Static helper method to rotate points.
  static gfx::PointF rotatePoint(const gfx::PointF& point,
                                 const gfx::PointF& pivot,
                                 const double angle,
                                 const double skew);

private:
  gfx::RectF m_bounds = gfx::RectF(0.0, 0.0, 0.0, 0.0);
  gfx::PointF m_pivot = gfx::PointF(0.0, 0.0);
  double m_angle = 0.0;
  double m_skew = 0.0;
  double m_cornerThick = CORNER_THICK_FOR_PIXELS_MODE;
};

} // namespace app

#endif
