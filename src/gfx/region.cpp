// Aseprite Gfx Library
// Copyright (C) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pixman.h"

#include "gfx/point.h"
#include "gfx/region.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace gfx {

inline Rect to_rect(const pixman_box32& extends)
{
  return Rect(
    extends.x1, extends.y1,
    extends.x2 - extends.x1,
    extends.y2 - extends.y1);
}

Region::Region()
{
  pixman_region32_init(&m_region);
}

Region::Region(const Region& copy)
{
  pixman_region32_init(&m_region);
  pixman_region32_copy(&m_region, &copy.m_region);
}

Region::Region(const Rect& rect)
{
  if (!rect.isEmpty())
    pixman_region32_init_rect(&m_region, rect.x, rect.y, rect.w, rect.h);
  else
    pixman_region32_init(&m_region);
}

Region::~Region()
{
  pixman_region32_fini(&m_region);
}

Region& Region::operator=(const Rect& rect)
{
  if (!rect.isEmpty()) {
    pixman_box32 box = { rect.x, rect.y, rect.x2(), rect.y2() };
    pixman_region32_reset(&m_region, &box);
  }
  else
    pixman_region32_clear(&m_region);
  return *this;
}

Region& Region::operator=(const Region& copy)
{
  pixman_region32_copy(&m_region, &copy.m_region);
  return *this;
}

Region::iterator Region::begin()
{
  iterator it;
  it.m_ptr = pixman_region32_rectangles(&m_region, NULL);
  return it;
}

Region::iterator Region::end()
{
  iterator it;
  it.m_ptr = pixman_region32_rectangles(&m_region, NULL) + size();
  return it;
}

Region::const_iterator Region::begin() const
{
  const_iterator it;
  it.m_ptr = pixman_region32_rectangles(&m_region, NULL);
  return it;
}

Region::const_iterator Region::end() const
{
  const_iterator it;
  it.m_ptr = pixman_region32_rectangles(&m_region, NULL) + size();
  return it;
}

bool Region::isEmpty() const
{
  return pixman_region32_not_empty(&m_region) ? false: true;
}

Rect Region::bounds() const
{
  return to_rect(*pixman_region32_extents(&m_region));
}

std::size_t Region::size() const
{
  return pixman_region32_n_rects(&m_region);
}

void Region::clear()
{
  pixman_region32_clear(&m_region);
}

void Region::offset(int dx, int dy)
{
  pixman_region32_translate(&m_region, dx, dy);
}

void Region::offset(const PointT<int>& delta)
{
  pixman_region32_translate(&m_region, delta.x, delta.y);
}

Region& Region::createIntersection(const Region& a, const Region& b)
{
  pixman_region32_intersect(&m_region, &a.m_region, &b.m_region);
  return *this;
}

Region& Region::createUnion(const Region& a, const Region& b)
{
  pixman_region32_union(&m_region, &a.m_region, &b.m_region);
  return *this;
}

Region& Region::createSubtraction(const Region& a, const Region& b)
{
  pixman_region32_subtract(&m_region, &a.m_region, &b.m_region);
  return *this;
}

bool Region::contains(const PointT<int>& pt) const
{
  return pixman_region32_contains_point(&m_region, pt.x, pt.y, NULL) ? true: false;
}

Region::Overlap Region::contains(const Rect& rect) const
{
  static_assert(
    int(Out)   == int(PIXMAN_REGION_OUT) &&
    int(In)    == int(PIXMAN_REGION_IN) &&
    int(Part)  == int(PIXMAN_REGION_PART), "Pixman constants have changed");

  pixman_box32 box = { rect.x, rect.y, rect.x2(), rect.y2() };
  return (Region::Overlap)pixman_region32_contains_rectangle(&m_region, &box);
}

Rect Region::operator[](int i)
{
  assert(i >= 0 && i < (int)size());
  return to_rect(pixman_region32_rectangles(&m_region, NULL)[i]);
}

const Rect Region::operator[](int i) const
{
  assert(i >= 0 && i < (int)size());
  return to_rect(pixman_region32_rectangles(&m_region, NULL)[i]);
}

} // namespace gfx
