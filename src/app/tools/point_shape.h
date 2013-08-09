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

#ifndef APP_TOOLS_TOOL_POINT_SHAPE_INCLUDED
#define APP_TOOLS_TOOL_POINT_SHAPE_H_INCLUDED

#include "gfx/rect.h"

namespace app {
  namespace tools {
    class ToolLoop;

    // Converts a point to a shape to be drawn
    class PointShape {
    public:
      virtual ~PointShape() { }
      virtual bool isFloodFill() { return false; }
      virtual bool isSpray() { return false; }
      virtual void transformPoint(ToolLoop* loop, int x, int y) = 0;
      virtual void getModifiedArea(ToolLoop* loop, int x, int y, gfx::Rect& area) = 0;

    protected:
      // Calls loop->getInk()->inkHline() function for each horizontal-scanline
      // that should be drawn (applying the "tiled" mode loop->getTiledMode())
      static void doInkHline(int x1, int y, int x2, ToolLoop* loop);
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_INK_H_INCLUDED
