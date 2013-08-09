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

#include <cmath>
#include <allegro.h>            // TODO avoid to include this for key[]

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

namespace app {
namespace tools {

using namespace gfx;

// Controls clicks for tools like pencil
class FreehandController : public Controller {
public:
  void pressButton(Points& points, const Point& point)
  {
    points.push_back(point);
  }
  bool releaseButton(Points& points, const Point& point)
  {
    return false;
  }
  void movement(ToolLoop* loop, Points& points, const Point& point)
  {
    points.push_back(point);
  }
  void getPointsToInterwine(const Points& input, Points& output)
  {
    if (input.size() == 1) {
      output.push_back(input[0]);
    }
    else if (input.size() >= 2) {
      output.push_back(input[input.size()-2]);
      output.push_back(input[input.size()-1]);
    }
  }
  void getStatusBarText(const Points& points, std::string& text)
  {
    ASSERT(!points.empty());
    if (points.empty())
      return;

    char buf[1024];
    sprintf(buf, "Start %3d %3d End %3d %3d",
            points[0].x, points[0].y,
            points[points.size()-1].x,
            points[points.size()-1].y);
    text = buf;
  }
};

// Controls clicks for tools like line
class TwoPointsController : public Controller {
  Point m_center;
public:
  void pressButton(Points& points, const Point& point)
  {
    m_center = point;

    points.push_back(point);
    points.push_back(point);
  }
  bool releaseButton(Points& points, const Point& point)
  {
    return false;
  }
  void movement(ToolLoop* loop, Points& points, const Point& point)
  {
    ASSERT(points.size() >= 2);
    if (points.size() < 2)
      return;

    points[1] = point;

    // Square aspect
    if (key[KEY_LSHIFT] || key[KEY_RSHIFT]) {
      int dx = points[1].x - m_center.x;
      int dy = points[1].y - m_center.y;
      int minsize = MIN(ABS(dx), ABS(dy));
      int maxsize = MAX(ABS(dx), ABS(dy));

      // Lines
      if (loop->getIntertwine()->snapByAngle()) {
        double angle = 180.0 * std::atan(static_cast<double>(-dy) /
                                         static_cast<double>(dx)) / M_PI;
        angle = ABS(angle);

        // Snap horizontally
        if (angle < 18.0) {
          points[1].y = m_center.y;
        }
        // Snap at 26.565
        else if (angle < 36.0) {
          points[1].x = m_center.x + SGN(dx)*maxsize;
          points[1].y = m_center.y + SGN(dy)*maxsize/2;
        }
        // Snap at 45
        else if (angle < 54.0) {
          points[1].x = m_center.x + SGN(dx)*minsize;
          points[1].y = m_center.y + SGN(dy)*minsize;
        }
        // Snap at 63.435
        else if (angle < 72.0) {
          points[1].x = m_center.x + SGN(dx)*maxsize/2;
          points[1].y = m_center.y + SGN(dy)*maxsize;
        }
        // Snap vertically
        else {
          points[1].x = m_center.x;
        }
      }
      // Rectangles and ellipses
      else {
        points[1].x = m_center.x + SGN(dx)*minsize;
        points[1].y = m_center.y + SGN(dy)*minsize;
      }
    }

    // Center
    if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) {
      int rx = points[1].x - m_center.x;
      int ry = points[1].y - m_center.y;
      points[0].x = m_center.x - rx;
      points[0].y = m_center.y - ry;
      points[1].x = m_center.x + rx;
      points[1].y = m_center.y + ry;
    }
    else
      points[0] = m_center;
  }
  void getPointsToInterwine(const Points& input, Points& output)
  {
    ASSERT(input.size() >= 2);
    if (input.size() < 2)
      return;

    output.push_back(input[0]);
    output.push_back(input[1]);
  }
  void getStatusBarText(const Points& points, std::string& text)
  {
    ASSERT(points.size() >= 2);
    if (points.size() < 2)
      return;

    char buf[1024];
    sprintf(buf, "Start %3d %3d End %3d %3d (Size %3d %3d) Angle %.1f",
            points[0].x, points[0].y,
            points[1].x, points[1].y,
            ABS(points[1].x-points[0].x)+1,
            ABS(points[1].y-points[0].y)+1,
            180.0 * std::atan2(static_cast<double>(points[0].y-points[1].y),
                               static_cast<double>(points[1].x-points[0].x)) / M_PI);
    text = buf;
  }
};

// Controls clicks for tools like polygon
class PointByPointController : public Controller {
public:
  void pressButton(Points& points, const Point& point)
  {
    points.push_back(point);
    points.push_back(point);
  }
  bool releaseButton(Points& points, const Point& point)
  {
    ASSERT(!points.empty());
    if (points.empty())
      return false;

    if (points[points.size()-2] == point &&
        points[points.size()-1] == point)
      return false;             // Click in the same point (no-drag), we are done
    else
      return true;              // Continue adding points
  }
  void movement(ToolLoop* loop, Points& points, const Point& point)
  {
    ASSERT(!points.empty());
    if (points.empty())
      return;

    points[points.size()-1] = point;
  }
  void getPointsToInterwine(const Points& input, Points& output)
  {
    output = input;
  }
  void getStatusBarText(const Points& points, std::string& text)
  {
    ASSERT(!points.empty());
    if (points.empty())
      return;

    char buf[1024];
    sprintf(buf, "Start %3d %3d End %3d %3d",
            points[0].x, points[0].y,
            points[points.size()-1].x,
            points[points.size()-1].y);
    text = buf;
  }
};

class OnePointController : public Controller {
public:
  // Do not apply grid to "one point tools" (e.g. magic wand, flood fill, etc.)
  bool canSnapToGrid() { return false; }

  void pressButton(Points& points, const Point& point)
  {
    if (points.size() == 0)
      points.push_back(point);
  }
  bool releaseButton(Points& points, const Point& point)
  {
    return false;
  }
  void movement(ToolLoop* loop, Points& points, const Point& point)
  {
    // Do nothing
  }
  void getPointsToInterwine(const Points& input, Points& output)
  {
    output = input;
  }
  void getStatusBarText(const Points& points, std::string& text)
  {
    ASSERT(!points.empty());
    if (points.empty())
      return;

    char buf[1024];
    sprintf(buf, "Pos %3d %3d", points[0].x, points[0].y);
    text = buf;
  }
};

class FourPointsController : public Controller {
  int m_clickCounter;
public:
  void pressButton(Points& points, const Point& point)
  {
    if (points.size() == 0) {
      points.resize(4, point);
      m_clickCounter = 0;
    }
    else
      m_clickCounter++;
  }
  bool releaseButton(Points& points, const Point& point)
  {
    m_clickCounter++;
    return m_clickCounter < 4;
  }
  void movement(ToolLoop* loop, Points& points, const Point& point)
  {
    switch (m_clickCounter) {
      case 0:
        for (size_t i=1; i<points.size(); ++i)
          points[i] = point;
        break;
      case 1:
      case 2:
        points[1] = point;
        points[2] = point;
        break;
      case 3:
        points[2] = point;
        break;
    }
  }
  void getPointsToInterwine(const Points& input, Points& output)
  {
    output = input;
  }
  void getStatusBarText(const Points& points, std::string& text)
  {
    ASSERT(points.size() >= 4);
    if (points.size() < 4)
      return;

    char buf[1024];
    sprintf(buf, "Start %3d %3d End %3d %3d (%3d %3d - %3d %3d)",
            points[0].x, points[0].y,
            points[3].x, points[3].y,
            points[1].x, points[1].y,
            points[2].x, points[2].y);

    text = buf;
  }
};

} // namespace tools
} // namespace app
