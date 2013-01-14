// ASEPRITE gfx library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gfx/region.h"
#include "gfx/point.h"
#include <limits>
#include <cassert>

// Anonymous namespace to include pixman-region.c. Macros doesn't respect
// namespaces but anyway they are defined inside it just for reference
// (to know that they are needed only for pixman-region.c).
namespace {

  #define PREFIX(x) pixman_region32##x
  #define PIXMAN_EXPORT
  #define _pixman_log_error(x, y) while (0) { }
  #define critical_if_fail assert

  typedef int64_t overflow_int_t;
  typedef gfx::details::Box box_type_t;
  typedef gfx::details::RegionData region_data_type_t;
  typedef gfx::details::Region region_type_t;
  typedef bool pixman_bool_t;

  #define UINT32_MAX        std::numeric_limits<uint32_t>::max()
  #define PIXMAN_REGION_MAX std::numeric_limits<int>::max()
  #define PIXMAN_REGION_MIN std::numeric_limits<int>::min()

  typedef gfx::Region::Overlap pixman_region_overlap_t;
  const gfx::Region::Overlap PIXMAN_REGION_OUT = gfx::Region::Out;
  const gfx::Region::Overlap PIXMAN_REGION_IN = gfx::Region::In;
  const gfx::Region::Overlap PIXMAN_REGION_PART = gfx::Region::Part;

  pixman_bool_t
  PREFIX(_union)(region_type_t *new_reg,
    region_type_t *reg1,
    region_type_t *reg2);

  #include "gfx/pixman/pixman-region.c"
}

using namespace gfx;
  
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
    box_type_t box = { rect.x, rect.y, rect.x2(), rect.y2() };
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
  it.m_ptr = PIXREGION_RECTS(&m_region);
  return it;
}

Region::iterator Region::end()
{
  iterator it;
  it.m_ptr = PIXREGION_RECTS(&m_region) + PIXREGION_NUMRECTS(&m_region);
  return it;
}

Region::const_iterator Region::begin() const
{
  const_iterator it;
  it.m_ptr = PIXREGION_RECTS(&m_region);
  return it;
}

Region::const_iterator Region::end() const
{
  const_iterator it;
  it.m_ptr = PIXREGION_RECTS(&m_region) + PIXREGION_NUMRECTS(&m_region);
  return it;
}

bool Region::isEmpty() const
{
  return pixman_region32_not_empty(&m_region) ? false: true;
}

Rect Region::getBounds() const
{
  return m_region.extents;
}

size_t Region::size() const
{
  return PIXREGION_NUMRECTS(&m_region);
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
  box_type_t box = { rect.x, rect.y, rect.x2(), rect.y2() };
  return pixman_region32_contains_rectangle(&m_region, &box);
}

Rect Region::operator[](int i)
{
  assert(i >= 0 && i < PIXREGION_NUMRECTS(&m_region));
  return Rect(PIXREGION_RECTS(&m_region)[i]);
}

const Rect Region::operator[](int i) const
{
  assert(i >= 0 && i < PIXREGION_NUMRECTS(&m_region));
  return Rect(PIXREGION_RECTS(&m_region)[i]);
}
