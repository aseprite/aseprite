// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/transformation.h"

#include "fixmath/fixmath.h"
#include "gfx/point.h"
#include "gfx/size.h"

#include <cmath>

namespace app {

using namespace gfx;

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

Transformation::Transformation(const RectF& bounds)
  : m_bounds(bounds)
{
  m_angle = 0.0;
  m_pivot.x = bounds.x + bounds.w/2;
  m_pivot.y = bounds.y + bounds.h/2;
}

void Transformation::transformBox(Corners& corners) const
{
  // TODO We could create a composed 4x4 matrix with all
  // transformation and apply the same matrix to avoid calling
  // rotatePoint/cos/sin functions 4 times, anyway, it's not
  // critical at this point.

  corners = m_bounds;
  for (std::size_t c=0; c<corners.size(); ++c)
    corners[c] = Transformation::rotatePoint(corners[c], m_pivot, m_angle);
}

void Transformation::displacePivotTo(const PointF& newPivot)
{
  // Calculate the rotated corners
  Corners corners;
  transformBox(corners);

  // Rotate-back (-angle) the position of the rotated origin (corners[0])
  // using the new pivot.
  PointF newBoundsOrigin =
    rotatePoint(corners.leftTop(),
                newPivot,
                -m_angle);

  // Change the new pivot.
  m_pivot = newPivot;
  m_bounds = RectF(newBoundsOrigin, m_bounds.size());
}

PointF Transformation::rotatePoint(
  const PointF& point,
  const PointF& pivot,
  double angle)
{
  using namespace fixmath;

  fixed fixangle = ftofix(-angle);
  fixangle = fixmul(fixangle, radtofix_r);

  fixed cos = fixcos(fixangle);
  fixed sin = fixsin(fixangle);
  fixed dx = fixsub(ftofix(point.x), ftofix(pivot.x));
  fixed dy = fixsub(ftofix(point.y), ftofix(pivot.y));
  return PointF(
    fixtof(fixadd(ftofix(pivot.x), fixsub(fixmul(dx, cos), fixmul(dy, sin)))),
    fixtof(fixadd(ftofix(pivot.y), fixadd(fixmul(dy, cos), fixmul(dx, sin)))));
}

RectF Transformation::transformedBounds() const
{
  // Get transformed corners
  Corners corners;
  transformBox(corners);

  // Create a union of all corners
  RectF bounds;
  for (int i=0; i<Corners::NUM_OF_CORNERS; ++i)
    bounds = bounds.createUnion(RectF(corners[i].x, corners[i].y, 1, 1));

  return bounds;
}

} // namespace app
