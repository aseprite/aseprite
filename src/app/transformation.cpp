// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/transformation.h"

#include "gfx/point.h"
#include "gfx/size.h"

#include <cmath>

namespace app {

using namespace gfx;

Transformation::Transformation()
{
}

Transformation::Transformation(const RectF& bounds, double cornerThick)
  : m_bounds(bounds)
  , m_cornerThick(cornerThick)
{
  m_pivot.x = bounds.x + bounds.w / 2;
  m_pivot.y = bounds.y + bounds.h / 2;
}

Transformation::Corners Transformation::transformedCorners() const
{
  Corners corners(m_bounds);

  // TODO We could create a composed 4x4 matrix with all
  // transformation and apply the same matrix to avoid calling
  // rotatePoint/cos/sin functions 4 times, anyway, it's not
  // critical at this point.

  for (std::size_t c = 0; c < corners.size(); ++c)
    corners[c] = Transformation::rotatePoint(corners[c], m_pivot, m_angle, m_skew);
  return corners;
}

void Transformation::displacePivotTo(const PointF& newPivot)
{
  // Calculate the rotated corners
  Corners corners = transformedCorners();

  // Rotate-back the position of the rotated origin (corners[0]) using
  // the new pivot.
  PointF pt = corners.leftTop();
  pt = rotatePoint(pt, newPivot, -m_angle, 0.0);
  pt = rotatePoint(pt, newPivot, 0.0, -m_skew);

  // Change the new pivot.
  m_pivot = newPivot;
  m_bounds = RectF(pt, m_bounds.size());
}

PointF Transformation::rotatePoint(const PointF& point,
                                   const PointF& pivot,
                                   const double angle,
                                   const double skew)
{
  double cos = std::roundl(std::cos(-angle) * 100000.0) / 100000.0;
  double sin = std::roundl(std::sin(-angle) * 100000.0) / 100000.0;
  double tan = std::roundl(std::tan(skew) * 100000.0) / 100000.0;
  double dx = point.x - pivot.x;
  double dy = point.y - pivot.y;
  dx += dy * tan;
  return PointF(pivot.x + dx * cos - dy * sin, pivot.y + dx * sin + dy * cos);
}

RectF Transformation::transformedBounds() const
{
  // Get transformed corners
  Corners corners = transformedCorners();

  // Create a union of all corners
  RectF bounds;
  for (int i = 0; i < Corners::NUM_OF_CORNERS; ++i)
    bounds = bounds.createUnion(RectF(corners[i].x, corners[i].y, m_cornerThick, m_cornerThick));

  return bounds.floor();
}

} // namespace app
