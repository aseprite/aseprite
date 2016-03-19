// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
  gfx::Point newPoint;
  div_t d, dx, dy;

  dx = std::div(grid.x, grid.w);
  dy = std::div(grid.y, grid.h);

  switch (prefer) {

    case PreferSnapTo::ClosestGridVertex:
      d = std::div(point.x-dx.rem, grid.w);
      newPoint.x = dx.rem + d.quot*grid.w + ((d.rem > grid.w/2)? grid.w: 0);

      d = std::div(point.y-dy.rem, grid.h);
      newPoint.y = dy.rem + d.quot*grid.h + ((d.rem > grid.h/2)? grid.h: 0);
      break;

    case PreferSnapTo::BoxOrigin:
      d = std::div(point.x-dx.rem, grid.w);
      newPoint.x = dx.rem + d.quot*grid.w;

      d = std::div(point.y-dy.rem, grid.h);
      newPoint.y = dy.rem + d.quot*grid.h;
      break;
  }

  return newPoint;
}

} // namespace app
