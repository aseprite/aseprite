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

#ifndef APP_TOOLS_INTERTWINE_H_INCLUDED
#define APP_TOOLS_INTERTWINE_H_INCLUDED

#include "gfx/point.h"

#include <vector>

namespace app {
  namespace tools {
    class ToolLoop;

    class Intertwine {
    public:
      typedef std::vector<gfx::Point> Points;

      virtual ~Intertwine() { }
      virtual bool snapByAngle() { return false; }
      virtual void prepareIntertwine() { }
      virtual void joinPoints(ToolLoop* loop, const Points& points) = 0;
      virtual void fillPoints(ToolLoop* loop, const Points& points) = 0;

    protected:
      static void doPointshapePoint(int x, int y, ToolLoop* loop);
      static void doPointshapeHline(int x1, int y, int x2, ToolLoop* loop);
      static void doPointshapeLine(int x1, int y1, int x2, int y2, ToolLoop* loop);
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_INTERTWINE_H_INCLUDED
