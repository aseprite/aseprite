// ASEPRITE gfx library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef TRANSFORMATION_H_INCLUDED
#define TRANSFORMATION_H_INCLUDED

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

      size_t size() const { return m_corners.size(); }

      PointT<double>& operator[](int index) { return m_corners[index]; }
      const PointT<double>& operator[](int index) const { return m_corners[index]; }

      const PointT<double>& leftTop() const { return m_corners[LEFT_TOP]; }
      const PointT<double>& rightTop() const { return m_corners[RIGHT_TOP]; }
      const PointT<double>& rightBottom() const { return m_corners[RIGHT_BOTTOM]; }
      const PointT<double>& leftBottom() const { return m_corners[LEFT_BOTTOM]; }

      void leftTop(const PointT<double>& pt) { m_corners[LEFT_TOP] = pt; }
      void rightTop(const PointT<double>& pt) { m_corners[RIGHT_TOP] = pt; }
      void rightBottom(const PointT<double>& pt) { m_corners[RIGHT_BOTTOM] = pt; }
      void leftBottom(const PointT<double>& pt) { m_corners[LEFT_BOTTOM] = pt; }

      Corners& operator=(const gfx::Rect& bounds) {
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

    private:
      std::vector<PointT<double> > m_corners;
  };

  Transformation();
  Transformation(const Rect& bounds);

  // Simple getters and setters. The angle is in radians.

  const Rect& bounds() const { return m_bounds; }
  const Point& pivot() const { return m_pivot; }
  double angle() const { return m_angle; }

  void bounds(const Rect& bounds) { m_bounds = bounds; }
  void pivot(const Point& pivot) { m_pivot = pivot; }
  void angle(double angle) { m_angle = angle; }

  // Applies the transformation (rotation with angle/pivot) to the
  // current bounds (m_bounds).
  void transformBox(Corners& corners) const;

  // Changes the pivot to another location, adjusting the bounds to
  // keep the current rotated-corners in the same location.
  void displacePivotTo(const Point& newPivot);

  // Static helper method to rotate points.
  static PointT<double> rotatePoint(const PointT<double>& point,
    const PointT<double>& pivot, double angle);

private:
  Rect m_bounds;
  Point m_pivot;
  double m_angle;
};

} // namespace gfx

#endif
