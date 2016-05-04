// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/point_shape.h"

#include "app/tools/ink.h"
#include "app/tools/tool_loop.h"
#include "app/util/wrap_value.h"
#include "doc/image.h"

namespace app {
namespace tools {

using namespace doc;
using namespace filters;

void PointShape::doInkHline(int x1, int y, int x2, ToolLoop* loop)
{
  TiledMode tiledMode = loop->getTiledMode();
  int x, w, size; // width or height

  // In case the ink needs original cel coordinates, we have to
  // translate the x1/y/x2 coordinate.
  if (loop->getInk()->needsCelCoordinates()) {
    gfx::Point origin = loop->getCelOrigin();
    x1 -= origin.x;
    x2 -= origin.x;
    y -= origin.y;
  }

  // Tiled in Y axis
  if (int(tiledMode) & int(TiledMode::Y_AXIS)) {
    size = loop->getDstImage()->height();      // size = image height
    y = wrap_value(y, size);
  }
  else if (y < 0 || y >= loop->getDstImage()->height())
    return;

  // Tiled in X axis
  if (int(tiledMode) & int(TiledMode::X_AXIS)) {
    if (x1 > x2)
      return;

    size = loop->getDstImage()->width();      // size = image width
    w = x2-x1+1;
    if (w >= size)
      loop->getInk()->inkHline(0, y, size-1, loop);
    else {
      x = x1;
      x = wrap_value(x, size);

      if (x+w-1 <= size-1)
        loop->getInk()->inkHline(x, y, x+w-1, loop);
      else {
        loop->getInk()->inkHline(x, y, size-1, loop);
        loop->getInk()->inkHline(0, y, w-(size-x)-1, loop);
      }
    }
  }
  // Clipped in X axis
  else {
    if (x1 < 0)
      x1 = 0;

    if (x2 >= loop->getDstImage()->width())
      x2 = loop->getDstImage()->width()-1;

    if (x2-x1+1 < 1)
      return;

    loop->getInk()->inkHline(x1, y, x2, loop);
  }
}

} // namespace tools
} // namespace app
