// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/point_shape.h"

#include "app/tools/ink.h"
#include "app/tools/tool_loop.h"
#include "app/util/wrap_value.h"
#include "doc/brush.h"
#include "doc/image.h"
#include "doc/sprite.h"

#include <algorithm>

namespace app {
namespace tools {

using namespace doc;
using namespace filters;

void PointShape::doInkHline(int x1, int y, int x2, ToolLoop* loop)
{
  Ink* ink = loop->getInk();
  TiledMode tiledMode = loop->getTiledMode();
  const int dstw = loop->getDstImage()->width();
  const int dsth = loop->getDstImage()->height();
  int x, w, size; // width or height

  // Without this fix, slice preview won't show when tilemap mode
  // is 'tiles' and slicing at a negative grid index
  gfx::Point limit(0,0);
  if (ink->isSlice() && loop->getPointShape()->isTile())
    limit = -loop->getGrid().origin();

  // In case the ink needs original cel coordinates, we have to
  // translate the x1/y/x2 coordinate.
  if (loop->needsCelCoordinates()) {
    gfx::Point origin = loop->getCelOrigin();
    x1 -= origin.x;
    x2 -= origin.x;
    y -= origin.y;
  }

  // Tiled in Y axis
  if (int(tiledMode) & int(TiledMode::Y_AXIS)) {
    size = dsth;      // size = image height
    y = wrap_value(y, size);
  }
  else if (y < limit.y || y >= dsth) {
    return;
  }

  // Tiled in X axis
  if (int(tiledMode) & int(TiledMode::X_AXIS)) {
    if (x1 > x2)
      return;

    size = dstw;      // size = image width
    w = x2-x1+1;
    if (w >= size)
      ink->inkHline(0, y, size-1, loop);
    else {
      x = wrap_value(x1, dstw);
      if (x+w <= dstw) {
        // Here we asure that tile limit line does not bisect the current
        // scanline, i.e. the scanline is enterely contained inside the tile.
        ink->prepareUForPointShapeWholeScanline(loop, x1);
        ink->inkHline(x, y, x+w-1, loop);
      }
      else {
        // Here the tile limit line bisect the current scanline.
        // So we need to execute TWO times the inkHline function, each one with a different m_u.
        ink->prepareUForPointShapeSlicedScanline(loop, true, x1);// true = left slice
        ink->inkHline(x, y, size-1, loop);

        ink->prepareUForPointShapeSlicedScanline(loop, false, x1);// false = right slice
        ink->inkHline(0, y, w-(size-x)-1, loop);
      }
    }
  }
  // Clipped in X axis
  else {
    if (x2 < limit.x || x1 >= dstw || x2-x1+1 < limit.x+1)
      return;

    x1 = std::clamp(x1, limit.x, dstw-1);
    x2 = std::clamp(x2, limit.x, dstw-1);
    ink->inkHline(x1, y, x2, loop);
  }
}

} // namespace tools
} // namespace app
