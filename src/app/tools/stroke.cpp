// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/stroke.h"

namespace app {
namespace tools {

void Stroke::reset()
{
  m_pts.clear();
}

void Stroke::reset(int n, const Pt& pt)
{
  m_pts.resize(n, pt);
}

void Stroke::addPoint(const Pt& pt)
{
  m_pts.push_back(pt);
}

void Stroke::offset(const gfx::Point& delta)
{
  for (auto& p : m_pts) {
    p.x += delta.x;
    p.y += delta.y;
  }
}

void Stroke::erase(int index)
{
  ASSERT(0 <= index && index < m_pts.size());
  if (0 <= index && index < m_pts.size())
    m_pts.erase(m_pts.begin()+index);
}

gfx::Rect Stroke::bounds() const
{
  if (m_pts.empty())
    return gfx::Rect();

  gfx::Point
    minpt(m_pts[0].x, m_pts[0].y),
    maxpt(m_pts[0].x, m_pts[0].y);

  for (std::size_t c=1; c<m_pts.size(); ++c) {
    int x = m_pts[c].x;
    int y = m_pts[c].y;
    if (minpt.x > x) minpt.x = x;
    if (minpt.y > y) minpt.y = y;
    if (maxpt.x < x) maxpt.x = x;
    if (maxpt.y < y) maxpt.y = y;
  }

  return gfx::Rect(minpt.x, minpt.y,
                   maxpt.x - minpt.x + 1,
                   maxpt.y - minpt.y + 1);
}

std::vector<int> Stroke::toXYInts() const
{
  std::vector<int> output;
  if (!empty()) {
    output.reserve(2*size());
    for (auto pt : m_pts) {
      output.push_back(pt.x);
      output.push_back(pt.y);
    }
  }
  return output;
}

} // namespace tools
} // namespace app
