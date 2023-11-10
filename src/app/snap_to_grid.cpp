// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/snap_to_grid.h"

#include "gfx/point.h"
#include "gfx/rect.h"

#include <cstdlib>

namespace app {

gfx::Point snap_to_grid(const gfx::Rect& grid,
                        const gfx::Point& point,
                        const PreferSnapTo prefer)
{
  if (grid.isEmpty())
    return point;

  div_t d, dx, dy;
  dx = std::div(grid.x, grid.w);
  dy = std::div(grid.y, grid.h);

  gfx::Point newPoint(point.x-dx.rem,
                      point.y-dy.rem);
  if (prefer != PreferSnapTo::ClosestGridVertex) {
    if (newPoint.x < 0) newPoint.x -= grid.w;
    if (newPoint.y < 0) newPoint.y -= grid.h;
  }

  switch (prefer) {

    case PreferSnapTo::ClosestGridVertex:
      d = std::div(newPoint.x, grid.w);
      newPoint.x = dx.rem + d.quot*grid.w + ((d.rem > grid.w/2)? grid.w: 0);

      d = std::div(newPoint.y, grid.h);
      newPoint.y = dy.rem + d.quot*grid.h + ((d.rem > grid.h/2)? grid.h: 0);
      break;

    case PreferSnapTo::BoxOrigin:
    case PreferSnapTo::FloorGrid:
      d = std::div(newPoint.x, grid.w);
      newPoint.x = dx.rem + d.quot*grid.w;

      d = std::div(newPoint.y, grid.h);
      newPoint.y = dy.rem + d.quot*grid.h;
      break;

    case PreferSnapTo::CeilGrid:
      d = std::div(newPoint.x, grid.w);
      newPoint.x = d.rem ? dx.rem + (d.quot+1)*grid.w: newPoint.x;

      d = std::div(newPoint.y, grid.h);
      newPoint.y = d.rem ? dy.rem + (d.quot+1)*grid.h: newPoint.y;
      break;

    case PreferSnapTo::BoxEnd:
      d = std::div(newPoint.x, grid.w);
      newPoint.x = dx.rem + (d.quot+1)*grid.w;

      d = std::div(newPoint.y, grid.h);
      newPoint.y = dy.rem + (d.quot+1)*grid.h;
      break;

    case PreferSnapTo::BoxCenter:
      d = std::div(newPoint.x, grid.w);
      bool nextBox = newPoint.x >= d.quot * grid.w;
      newPoint.x = d.quot * grid.w + grid.w / (nextBox ? 2 : -2);

      d = std::div(newPoint.y, grid.h);
      nextBox = newPoint.y >= d.quot * grid.h;
      newPoint.y = d.quot * grid.h + grid.h / (nextBox ? 2 : -2);
      break;
  }

  return newPoint;
}

} // namespace app
