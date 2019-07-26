// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/color_curve.h"

#include <algorithm>

namespace filters {

ColorCurve::ColorCurve(Type type)
  : m_type(type)
{
}

void ColorCurve::addDefaultPoints()
{
  addPoint(gfx::Point(0, 0));
  addPoint(gfx::Point(255, 255));
}

void ColorCurve::addPoint(const gfx::Point& point)
{
  for (iterator it = begin(), end = this->end(); it != end; ++it) {
    if (it->x >= point.x) {
      m_points.insert(it, point);
      return;
    }
  }
  m_points.push_back(point);
}

void ColorCurve::removePoint(const gfx::Point& point)
{
  iterator it = std::find(begin(), end(), point);
  if (it != end())
    m_points.erase(it);
}

void ColorCurve::getValues(int x1, int x2, std::vector<int>& values)
{
  int x, num_points = (int)m_points.size();

  if (num_points == 0) {
    for (x=x1; x<=x2; x++)
      values[x-x1] = 0;
  }
  else if (num_points == 1) {
    for (x=x1; x<=x2; x++)
      values[x-x1] = m_points[0].y;
  }
  else {
    switch (m_type) {

      case Linear: {
        iterator it, begin = this->begin(), end = this->end();

        for (it = begin; it != end; ++it)
          if (it->x >= x1)
            break;

        for (x=x1; x<=x2; x++) {
          while ((it != end) && (x > it->x))
            ++it;

          if (it != end) {
            if (it != begin) {
              const gfx::Point& p = *(it-1);
              const gfx::Point& n = *it;

              values[x-x1] = p.y + (n.y-p.y) * (x-p.x) / (n.x-p.x);
            }
            else {
              values[x-x1] = it->y;
            }
          }
          else {
            values[x-x1] = (end-1)->y;
          }
        }
        break;
      }

    }
  }
}

} // namespace filters
