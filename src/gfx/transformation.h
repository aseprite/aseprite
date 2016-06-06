// Aseprite Gfx Library
// Copyright (C) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef TRANSFORMATION_H_INCLUDED
#define TRANSFORMATION_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"
#include <vector>

namespace gfx {

// Represents a transformation that can be done by the user in the
// document when he/she moves the mask using the selection handles.
class Transformation
{
public:
  class Corners {
    public:
      enum {
        LEFT_TOP = 0,
        RIGHT_TOP = 1,
        RIGHT_BOTTOM = 2,
        LEFT_BOTTOM = 3,
        NUM_OF_CORNERS = 4
      };

      Corners() : m_corners(NUM_OF_CORNERS) { }

      std::size_t size() const { return m_corners.size(); }

      PointF& operator[](int index) { return m_corners[index]; }
      const PointF& operator[](int index) const { return m_corners[index]; }

      const PointF& leftTop() const { return m_corners[LEFT_TOP]; }
      const PointF& rightTop() const { return m_corners[RIGHT_TOP]; }
      const PointF& rightBottom() const { return m_corners[RIGHT_BOTTOM]; }
      const PointF& leftBottom() const { return m_corners[LEFT_BOTTOM]; }

      void leftTop(const PointF& pt) { m_corners[LEFT_TOP] = pt; }
      void rightTop(const PointF& pt) { m_corners[RIGHT_TOP] = pt; }
      void rightBottom(const PointF& pt) { m_corners[RIGHT_BOTTOM] = pt; }
      void leftBottom(const PointF& pt) { m_corners[LEFT_BOTTOM] = pt; }

      Corners& operator=(const RectF bounds) {
        m_corners[LEFT_TOP].x = bounds.x;
        m_corners[LEFT_TOP].y = bounds.y;
        m_corners[RIGHT_TOP].x = bounds.x + bounds.w;
        m_corners[RIGHT_TOP].y = bounds.y;
        m_corners[RIGHT_BOTTOM].x = bounds.x + bounds.w;
        m_corners[RIGHT_BOTTOM].y = bounds.y + bounds.h;
        m_corners[LEFT_BOTTOM].x = bounds.x;
        m_corners[LEFT_BOTTOM].y = bounds.y + bounds.h;
        return *this;
      }

      RectF bounds() const {
        RectF bounds;
        for (int i=0; i<Corners::NUM_OF_CORNERS; ++i)
          bounds |= RectF(m_corners[i].x, m_corners[i].y, 1, 1);
        return bounds;
      }

    private:
      std::vector<PointF> m_corners;
  };

  Transformation();
  Transformation(const RectF& bounds);

  // Simple getters and setters. The angle is in radians.

  const RectF& bounds() const { return m_bounds; }
  const PointF& pivot() const { return m_pivot; }
  double angle() const { return m_angle; }

  void bounds(const RectF& bounds) { m_bounds = bounds; }
  void pivot(const PointF& pivot) { m_pivot = pivot; }
  void angle(double angle) { m_angle = angle; }

  // Applies the transformation (rotation with angle/pivot) to the
  // current bounds (m_bounds).
  void transformBox(Corners& corners) const;

  // Changes the pivot to another location, adjusting the bounds to
  // keep the current rotated-corners in the same location.
  void displacePivotTo(const PointF& newPivot);

  RectF transformedBounds() const;

  // Static helper method to rotate points.
  static PointF rotatePoint(const PointF& point,
                            const PointF& pivot,
                            double angle);

private:
  RectF m_bounds;
  PointF m_pivot;
  double m_angle;
};

} // namespace gfx

#endif
