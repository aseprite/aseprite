// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/snap_to_grid.h"

#include "app/app.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/pref/preferences.h"
#include "doc/grid.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <cstdlib>

namespace app {

gfx::Point snap_to_isometric_grid(const gfx::Rect& grid,
                                  const gfx::Point& point,
                                  const PreferSnapTo prefer)
{
  // Convert point to grid space
  gfx::PointF newPoint(int((point.x - grid.x) / double(grid.w)) * grid.w,
                       int((point.y - grid.y) / double(grid.h)) * grid.h);

  // Substract this from original point (also in grid space)
  // to obtain newPoint as an offset within the first grid cell
  const gfx::PointF diff((point - grid.origin()) - newPoint);

  // We now find the closest corner to that offset
  const gfx::PointF candidates[] = { gfx::PointF(grid.w * 0.5, 0),
                                     gfx::PointF(grid.w * 0.5, grid.h),
                                     gfx::PointF(0, grid.h * 0.5),
                                     gfx::PointF(grid.w, grid.h * 0.5) };
  gfx::PointF near(grid.origin());
  double dist = (grid.w + grid.h) * 2;
  for (const auto& c : candidates) {
    gfx::PointF vto = diff - c;
    if (vto.x < 0)
      vto.x = -vto.x;
    if (vto.y < 0)
      vto.y = -vto.y;
    const double newDist = vto.x + vto.y;
    if (newDist < dist) {
      near = c;
      dist = newDist;
    }
  }

  // TODO: translate the use of the 'prefer' argument from
  //       the orthogonal logic to this function

  // Convert cell offset to pixel space
  newPoint += near + grid.origin();
  return gfx::Point(std::round(newPoint.x), std::round(newPoint.y));
}

gfx::Point snap_to_grid(const gfx::Rect& grid, const gfx::Point& point, const PreferSnapTo prefer)
{
  if (grid.isEmpty())
    return point;

  // Use different logic for isometric grid
  const doc::Grid::Type gridType =
    App::instance()->preferences().document(App::instance()->context()->documents()[0]).grid.type();
  if (gridType == doc::Grid::Type::Isometric)
    return snap_to_isometric_grid(grid, point, prefer);

  div_t d, dx, dy;
  dx = std::div(grid.x, grid.w);
  dy = std::div(grid.y, grid.h);

  gfx::Point newPoint(point.x - dx.rem, point.y - dy.rem);
  if (prefer != PreferSnapTo::ClosestGridVertex) {
    if (newPoint.x < 0)
      newPoint.x -= grid.w;
    if (newPoint.y < 0)
      newPoint.y -= grid.h;
  }

  switch (prefer) {
    case PreferSnapTo::ClosestGridVertex:
      d = std::div(newPoint.x, grid.w);
      newPoint.x = dx.rem + d.quot * grid.w + ((d.rem > grid.w / 2) ? grid.w : 0);

      d = std::div(newPoint.y, grid.h);
      newPoint.y = dy.rem + d.quot * grid.h + ((d.rem > grid.h / 2) ? grid.h : 0);
      break;

    case PreferSnapTo::BoxOrigin:
    case PreferSnapTo::FloorGrid:
      d = std::div(newPoint.x, grid.w);
      newPoint.x = dx.rem + d.quot * grid.w;

      d = std::div(newPoint.y, grid.h);
      newPoint.y = dy.rem + d.quot * grid.h;
      break;

    case PreferSnapTo::CeilGrid:
      d = std::div(newPoint.x, grid.w);
      newPoint.x = d.rem ? dx.rem + (d.quot + 1) * grid.w : newPoint.x;

      d = std::div(newPoint.y, grid.h);
      newPoint.y = d.rem ? dy.rem + (d.quot + 1) * grid.h : newPoint.y;
      break;

    case PreferSnapTo::BoxEnd:
      d = std::div(newPoint.x, grid.w);
      newPoint.x = dx.rem + (d.quot + 1) * grid.w;

      d = std::div(newPoint.y, grid.h);
      newPoint.y = dy.rem + (d.quot + 1) * grid.h;
      break;
  }

  return newPoint;
}

} // namespace app
