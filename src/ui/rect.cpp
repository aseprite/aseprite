// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/rect.h"

namespace ui {

JRect jrect_new(int x1, int y1, int x2, int y2)
{
  JRect rect = new jrect;

  rect->x1 = x1;
  rect->y1 = y1;
  rect->x2 = x2;
  rect->y2 = y2;

  return rect;
}

JRect jrect_new_copy(const JRect rect)
{
  return jrect_new(rect->x1, rect->y1, rect->x2, rect->y2);
}

void jrect_free(JRect rect)
{
  delete rect;
}

void jrect_copy(JRect dst, const JRect src)
{
  dst->x1 = src->x1;
  dst->y1 = src->y1;
  dst->x2 = src->x2;
  dst->y2 = src->y2;
}

void jrect_replace(JRect rect, int x1, int y1, int x2, int y2)
{
  rect->x1 = x1;
  rect->y1 = y1;
  rect->x2 = x2;
  rect->y2 = y2;
}

void jrect_union(JRect r1, const JRect r2)
{
  r1->x1 = MIN(r1->x1, r2->x1);
  r1->y1 = MIN(r1->y1, r2->y1);
  r1->x2 = MAX(r1->x2, r2->x2);
  r1->y2 = MAX(r1->y2, r2->y2);
}

bool jrect_intersect(JRect r1, const JRect r2)
{
  if (!((r1->x2 <= r2->x1) || (r1->x1 >= r2->x2) ||
        (r1->y2 <= r2->y1) || (r1->y1 >= r2->y2))) {
    r1->x1 = MAX(r1->x1, r2->x1);
    r1->y1 = MAX(r1->y1, r2->y1);
    r1->x2 = MIN(r1->x2, r2->x2);
    r1->y2 = MIN(r1->y2, r2->y2);
    return true;
  }
  else
    return false;
}

void jrect_shrink(JRect rect, int border)
{
  rect->x1 += border;
  rect->y1 += border;
  rect->x2 -= border;
  rect->y2 -= border;
}

void jrect_stretch(JRect rect, int border)
{
  rect->x1 -= border;
  rect->y1 -= border;
  rect->x2 += border;
  rect->y2 += border;
}

void jrect_moveto(JRect rect, int x, int y)
{
  rect->x2 += x-rect->x1;
  rect->y2 += y-rect->y1;
  rect->x1 = x;
  rect->y1 = y;
}

void jrect_displace(JRect rect, int dx, int dy)
{
  rect->x1 += dx;
  rect->y1 += dy;
  rect->x2 += dx;
  rect->y2 += dy;
}

} // namespace ui
