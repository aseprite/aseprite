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
  // Because we force unworkable grid sizes to share a pixel,
  // we need to account for that here
  auto guide = doc::Grid(grid).getIsometricLinePoints();
  const int width = guide[2].x;
  int height = guide[2].y;

  if (ABS(grid.w - grid.h) > 1) {
    const bool x_share = (guide[1].x & 1) != 0 && (grid.w & 1) == 0;
    const bool y_share = ((guide[0].y & 1) == 0 || (grid.w & 1) == 0) && (grid.h & 1) != 0;
    const bool y_undiv = ((grid.h / 2) & 1) != 0;
    const bool y_uneven = (grid.w & 1) != 0 && (grid.h & 1) == 0;
    const bool y_skip = !x_share && !y_undiv && !y_uneven && (grid.w & 1) != 0 && (grid.h & 1) != 0;
    if (x_share) {
      guide[1].x++;
    }
    if (y_share && !y_skip) {
      guide[0].y--;
    }
    else {
      if (y_undiv) {
        height++;
      }
      if (y_uneven) {
        guide[0].y++;
        guide[1].x += int((grid.w & 1) == 0);
      }
    }
  }

  // Convert point to grid space
  const gfx::PointF newPoint(int((point.x - grid.x) / double(grid.w)) * grid.w,
                             int((point.y - grid.y) / double(grid.h)) * grid.h);
  // And then make it relative to the center of a cell
  const gfx::PointF vto((newPoint + gfx::Point(guide[1].x, guide[0].y)) - point);

  // The following happens here:
  //
  //  /\  /\
  // /A \/B \
  // \  /\  /
  //  \/  \/
  //  /\  /\
  // /C \/D \
  //
  // Only the origin for diamonds (A,B,C,D) can be found by dividing
  // the original point by grid size.
  //
  // In order to snap to a position relative to the "in-between" diamonds,
  // we need to determine whether the cell coords are outside the
  // bounds of the current grid cell.
  bool outside = false;

  if (prefer != PreferSnapTo::ClosestGridVertex) {
    // We use the pixel-precise grid for this bounds-check
    const auto& line = doc::Grid(grid).getIsometricLine();
    const int index = int(ABS(vto.y) - int(vto.y > 0)) + 1;
    const gfx::Point co(-vto.x + guide[1].x, -vto.y + guide[0].y);
    const gfx::Point& p = line[index];
    outside = !(p.x <= co.x) || !(co.x < width - p.x) || !(height - p.y <= co.y) || !(co.y < p.y);
  }

  // Find which of the four corners of the current diamond
  // should be picked
  gfx::Point near(0, 0);
  const gfx::Point candidates[] = { gfx::Point(guide[1].x, 0),
                                    gfx::Point(guide[1].x, height),
                                    gfx::Point(0, guide[0].y),
                                    gfx::Point(width, guide[0].y) };
  switch (prefer) {
    case PreferSnapTo::ClosestGridVertex:
      if (ABS(vto.x) > ABS(vto.y))
        near = (vto.x < 0 ? candidates[3] : candidates[2]);
      else
        near = (vto.y < 0 ? candidates[1] : candidates[0]);
      break;

    // Pick topmost corner
    case PreferSnapTo::FloorGrid:
    case PreferSnapTo::BoxOrigin:
      if (outside) {
        near = (vto.x < 0 ? candidates[3] : candidates[2]);
        near.y -= (vto.y > 0 ? height : 0);
      }
      else {
        near = candidates[0];
      }
      break;

    // Pick bottom-most corner
    case PreferSnapTo::CeilGrid:
    case PreferSnapTo::BoxEnd:
      if (outside) {
        near = (vto.x < 0 ? candidates[3] : candidates[2]);
        near.y += (vto.y < 0 ? height : 0);
      }
      else {
        near = candidates[1];
      }
      break;
  }
  // Convert offset back to pixel space
  return gfx::Point(newPoint + near + grid.origin());
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
