// Aseprite Gfx Library
// Copyright (C) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/transformation.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "fixmath/fixmath.h"

#include <cmath>

namespace gfx {

Transformation::Transformation()
{
  m_bounds.x = 0;
  m_bounds.y = 0;
  m_bounds.w = 0;
  m_bounds.h = 0;
  m_angle = 0.0;
  m_pivot.x = 0;
  m_pivot.y = 0;
}

Transformation::Transformation(const Rect& bounds)
  : m_bounds(bounds)
{
  m_angle = 0.0;
  m_pivot.x = bounds.x + bounds.w/2;
  m_pivot.y = bounds.y + bounds.h/2;
}

void Transformation::transformBox(Corners& corners) const
{
  PointT<double> pivot_f(m_pivot);

  // TODO We could create a composed 4x4 matrix with all
  // transformation and apply the same matrix to avoid calling
  // rotatePoint/cos/sin functions 4 times, anyway, it's not
  // critical at this point.

  corners = m_bounds;
  for (std::size_t c=0; c<corners.size(); ++c)
    corners[c] = Transformation::rotatePoint(corners[c], pivot_f, m_angle);
}

void Transformation::displacePivotTo(const Point& newPivot)
{
  // Calculate the rotated corners
  Corners corners;
  transformBox(corners);

  // Rotate-back (-angle) the position of the rotated origin (corners[0])
  // using the new pivot.
  PointT<double> newBoundsOrigin =
    rotatePoint(corners.leftTop(),
                PointT<double>(newPivot.x, newPivot.y),
                -m_angle);

  // Change the new pivot.
  m_pivot = newPivot;
  m_bounds = Rect(Point(newBoundsOrigin), m_bounds.size());
}

PointT<double> Transformation::rotatePoint(
    const PointT<double>& point,
    const PointT<double>& pivot,
    double angle)
{
  using namespace fixmath;

  fixed fixangle = ftofix(-angle);
  fixangle = fixmul(fixangle, radtofix_r);

  fixed cos = fixcos(fixangle);
  fixed sin = fixsin(fixangle);
  fixed dx = fixsub(ftofix(point.x), ftofix(pivot.x));
  fixed dy = fixsub(ftofix(point.y), ftofix(pivot.y));
  return PointT<double>(
    fixtof(fixadd(ftofix(pivot.x), fixsub(fixmul(dx, cos), fixmul(dy, sin)))),
    fixtof(fixadd(ftofix(pivot.y), fixadd(fixmul(dy, cos), fixmul(dx, sin)))));
}

Rect Transformation::transformedBounds() const
{
  // Get transformed corners
  Corners corners;
  transformBox(corners);

  // Create a union of all corners
  Rect bounds;
  for (int i=0; i<Corners::NUM_OF_CORNERS; ++i)
    bounds = bounds.createUnion(gfx::Rect((int)corners[i].x, (int)corners[i].y, 1, 1));

  return bounds;
}

}
