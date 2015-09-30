// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "base/pi.h"

#include <cmath>

namespace app {
namespace tools {

using namespace gfx;

// Shared logic between controllers that can move/displace all points
// using the space bar.
class MoveOriginCapability : public Controller {
public:
  void prepareController(ui::KeyModifiers modifiers) override {
    m_movingOrigin = false;
  }

  void pressButton(Points& points, const Point& point) override {
    m_last = point;
  }

  bool pressKey(ui::KeyScancode key) override {
    TRACE("pressKey(%d)\n", key);
    return processKey(key, true);
  }

  bool releaseKey(ui::KeyScancode key) override {
    TRACE("releaseKey(%d)\n", key);
    return processKey(key, false);
  }

protected:
  bool isMovingOrigin(Points& points, const Point& point) {
    bool used = false;

    if (m_movingOrigin) {
      Point delta = (point - m_last);
      for (auto& p : points)
        p += delta;

      onMoveOrigin(delta);
      used = true;
    }

    m_last = point;
    return used;
  }

  virtual void onMoveOrigin(const Point& delta) {
    // Do nothing
  }

private:
  bool processKey(ui::KeyScancode key, bool state) {
    if (key == ui::kKeySpace) {
      m_movingOrigin = state;
      return true;
    }
    return false;
  }

  // Flag used to know if the space bar is pressed, i.e., we have
  // displace all points.
  bool m_movingOrigin;

  // Last known mouse position used to calculate delta values (dx, dy)
  // with the new mouse position to displace all points.
  Point m_last;
};

// Controls clicks for tools like pencil
class FreehandController : public Controller {
public:
  bool isFreehand() override { return true; }

  void pressButton(Points& points, const Point& point) override {
    points.push_back(point);
  }

  bool releaseButton(Points& points, const Point& point) override {
    return false;
  }

  void movement(ToolLoop* loop, Points& points, const Point& point) override {
    points.push_back(point);
  }

  void getPointsToInterwine(const Points& input, Points& output) override {
    if (input.size() == 1) {
      output.push_back(input[0]);
    }
    else if (input.size() >= 2) {
      output.push_back(input[input.size()-2]);
      output.push_back(input[input.size()-1]);
    }
  }

  void getStatusBarText(const Points& points, std::string& text) override {
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
class TwoPointsController : public MoveOriginCapability {
public:
  void prepareController(ui::KeyModifiers modifiers) override {
    MoveOriginCapability::prepareController(modifiers);

    m_squareAspect = (modifiers & ui::kKeyShiftModifier) ? true: false;
    m_fromCenter = (modifiers & ui::kKeyCtrlModifier) ? true: false;
  }

  void pressButton(Points& points, const Point& point) override {
    MoveOriginCapability::pressButton(points, point);

    m_first = point;

    points.push_back(point);
    points.push_back(point);
  }

  bool releaseButton(Points& points, const Point& point) override {
    return false;
  }

  bool pressKey(ui::KeyScancode key) override {
    if (MoveOriginCapability::pressKey(key))
      return true;

    return processKey(key, true);
  }

  bool releaseKey(ui::KeyScancode key) override {
    if (MoveOriginCapability::releaseKey(key))
      return true;

    return processKey(key, false);
  }

  void movement(ToolLoop* loop, Points& points, const Point& point) override {
    ASSERT(points.size() >= 2);
    if (points.size() < 2)
      return;

    if (MoveOriginCapability::isMovingOrigin(points, point))
      return;

    points[1] = point;

    if (m_squareAspect) {
      int dx = points[1].x - m_first.x;
      int dy = points[1].y - m_first.y;
      int minsize = MIN(ABS(dx), ABS(dy));
      int maxsize = MAX(ABS(dx), ABS(dy));

      // Lines
      if (loop->getIntertwine()->snapByAngle()) {
        double angle = 180.0 * std::atan(static_cast<double>(-dy) /
                                         static_cast<double>(dx)) / PI;
        angle = ABS(angle);

        // Snap horizontally
        if (angle < 18.0) {
          points[1].y = m_first.y;
        }
        // Snap at 26.565
        else if (angle < 36.0) {
          points[1].x = m_first.x + SGN(dx)*maxsize;
          points[1].y = m_first.y + SGN(dy)*maxsize/2;
        }
        // Snap at 45
        else if (angle < 54.0) {
          points[1].x = m_first.x + SGN(dx)*minsize;
          points[1].y = m_first.y + SGN(dy)*minsize;
        }
        // Snap at 63.435
        else if (angle < 72.0) {
          points[1].x = m_first.x + SGN(dx)*maxsize/2;
          points[1].y = m_first.y + SGN(dy)*maxsize;
        }
        // Snap vertically
        else {
          points[1].x = m_first.x;
        }
      }
      // Rectangles and ellipses
      else {
        points[1].x = m_first.x + SGN(dx)*minsize;
        points[1].y = m_first.y + SGN(dy)*minsize;
      }
    }

    points[0] = m_first;

    if (m_fromCenter) {
      int rx = points[1].x - m_first.x;
      int ry = points[1].y - m_first.y;
      points[0].x = m_first.x - rx;
      points[0].y = m_first.y - ry;
      points[1].x = m_first.x + rx;
      points[1].y = m_first.y + ry;
    }

    // Adjust points for selection like tools (so we can select tiles)
    if (loop->getController()->canSnapToGrid() &&
        loop->getSnapToGrid() &&
        loop->getInk()->isSelection()) {
      if (points[0].x < points[1].x)
        points[1].x--;
      else if (points[0].x > points[1].x)
        points[0].x--;

      if (points[0].y < points[1].y)
        points[1].y--;
      else if (points[0].y > points[1].y)
        points[0].y--;
    }
  }

  void getPointsToInterwine(const Points& input, Points& output) override {
    ASSERT(input.size() >= 2);
    if (input.size() < 2)
      return;

    output.push_back(input[0]);
    output.push_back(input[1]);
  }

  void getStatusBarText(const Points& points, std::string& text) override {
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
                               static_cast<double>(points[1].x-points[0].x)) / PI);
    text = buf;
  }

private:
  void onMoveOrigin(const Point& delta) override {
    m_first += delta;
  }

  bool processKey(ui::KeyScancode key, bool state) {
    switch (key) {
      case ui::kKeyLShift:
      case ui::kKeyRShift:
        m_squareAspect = state;
        return true;
      case ui::kKeyLControl:
      case ui::kKeyRControl:
        m_fromCenter = state;
        return true;
    }
    return false;
  }

  Point m_first;
  bool m_squareAspect;
  bool m_fromCenter;
};

// Controls clicks for tools like polygon
class PointByPointController : public MoveOriginCapability {
public:

  void pressButton(Points& points, const Point& point) override {
    MoveOriginCapability::pressButton(points, point);

    points.push_back(point);
    points.push_back(point);
  }

  bool releaseButton(Points& points, const Point& point) override {
    ASSERT(!points.empty());
    if (points.empty())
      return false;

    if (points[points.size()-2] == point &&
        points[points.size()-1] == point)
      return false;             // Click in the same point (no-drag), we are done
    else
      return true;              // Continue adding points
  }

  void movement(ToolLoop* loop, Points& points, const Point& point) override {
    ASSERT(!points.empty());
    if (points.empty())
      return;

    if (MoveOriginCapability::isMovingOrigin(points, point))
      return;

    points[points.size()-1] = point;
  }

  void getPointsToInterwine(const Points& input, Points& output) override {
    output = input;
  }

  void getStatusBarText(const Points& points, std::string& text) override {
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
  bool canSnapToGrid() override { return false; }
  bool isOnePoint() override { return true; }

  void pressButton(Points& points, const Point& point) override {
    if (points.size() == 0)
      points.push_back(point);
  }

  bool releaseButton(Points& points, const Point& point) override {
    return false;
  }

  void movement(ToolLoop* loop, Points& points, const Point& point) override {
    // Do nothing
  }

  void getPointsToInterwine(const Points& input, Points& output) override {
    output = input;
  }

  void getStatusBarText(const Points& points, std::string& text) override {
    ASSERT(!points.empty());
    if (points.empty())
      return;

    char buf[1024];
    sprintf(buf, "Pos %3d %3d", points[0].x, points[0].y);
    text = buf;
  }

};

class FourPointsController : public MoveOriginCapability {
public:

  void pressButton(Points& points, const Point& point) override {
    MoveOriginCapability::pressButton(points, point);

    if (points.size() == 0) {
      points.resize(4, point);
      m_clickCounter = 0;
    }
    else
      m_clickCounter++;
  }

  bool releaseButton(Points& points, const Point& point) override {
    m_clickCounter++;
    return m_clickCounter < 4;
  }

  void movement(ToolLoop* loop, Points& points, const Point& point) override {
    if (MoveOriginCapability::isMovingOrigin(points, point))
      return;

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

  void getPointsToInterwine(const Points& input, Points& output) override {
    output = input;
  }

  void getStatusBarText(const Points& points, std::string& text) override {
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

private:
  int m_clickCounter;
};

} // namespace tools
} // namespace app
