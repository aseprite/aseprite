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

#include "app/tools/intertwine.h"

#include "app/tools/point_shape.h"
#include "app/tools/tool_loop.h"
#include "doc/algo.h"

namespace app {
namespace tools {

using namespace gfx;
using namespace doc;

void Intertwine::doPointshapePoint(int x, int y, ToolLoop* loop)
{
  loop->getPointShape()->transformPoint(loop, x, y);
}

void Intertwine::doPointshapeHline(int x1, int y, int x2, ToolLoop* loop)
{
  algo_line(x1, y, x2, y, loop, (AlgoPixel)doPointshapePoint);
}

void Intertwine::doPointshapeLine(int x1, int y1, int x2, int y2, ToolLoop* loop)
{
  algo_line(x1, y1, x2, y2, loop, (AlgoPixel)doPointshapePoint);
}

} // namespace tools
} // namespace app
