/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/point_shape.h"

#include "app/settings/document_settings.h"
#include "app/tools/ink.h"
#include "app/tools/tool_loop.h"
#include "raster/image.h"

namespace app {
namespace tools {

using namespace raster;
using namespace filters;
  
void PointShape::doInkHline(int x1, int y, int x2, ToolLoop* loop)
{
  register TiledMode tiledMode = loop->getDocumentSettings()->getTiledMode();
  register int w, size; // width or height
  register int x;

  // Tiled in Y axis
  if (tiledMode & TILED_Y_AXIS) {
    size = loop->getDstImage()->h;      // size = image height
    if (y < 0)
      y = size - (-(y+1) % size) - 1;
    else
      y = y % size;
  }
  else if (y < 0 || y >= loop->getDstImage()->h)
      return;

  // Tiled in X axis
  if (tiledMode & TILED_X_AXIS) {
    if (x1 > x2)
      return;

    size = loop->getDstImage()->w;      // size = image width
    w = x2-x1+1;
    if (w >= size)
      loop->getInk()->inkHline(0, y, size-1, loop);
    else {
      x = x1;
      if (x < 0)
        x = size - (-(x+1) % size) - 1;
      else
        x = x % size;

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

    if (x2 >= loop->getDstImage()->w)
      x2 = loop->getDstImage()->w-1;

    if (x2-x1+1 < 1)
      return;

    loop->getInk()->inkHline(x1, y, x2, loop);
  }
}

} // namespace tools
} // namespace app
