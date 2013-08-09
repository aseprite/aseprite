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

#ifndef APP_TOOLS_CONTROLLER_H_INCLUDED
#define APP_TOOLS_CONTROLLER_H_INCLUDED

#include "gfx/point.h"

#include <string>
#include <vector>

namespace app {
  namespace tools {

    class ToolLoop;

    // This class controls user input.
    class Controller {
    public:
      typedef std::vector<gfx::Point> Points;

      virtual ~Controller() { }
      virtual bool canSnapToGrid() { return true; }

      // Called when the user starts drawing and each time a new button is
      // pressed. The controller could be sure that this method is called
      // at least one time.
      virtual void pressButton(Points& points, const gfx::Point& point) = 0;

      // Called each time a mouse button is released.
      virtual bool releaseButton(Points& points, const gfx::Point& point) = 0;

      // Called when the mouse is moved.
      virtual void movement(ToolLoop* loop, Points& points, const gfx::Point& point) = 0;

      virtual void getPointsToInterwine(const Points& input, Points& output) = 0;
      virtual void getStatusBarText(const Points& points, std::string& text) = 0;
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_TOOL_INK_H_INCLUDED
