// ASE gfx library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "gfx/rect.h"
#include "gfx/point.h"
#include "gfx/size.h"

#ifdef WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

using namespace gfx;

Rect::Rect()
{
  x = 0;
  y = 0;
  w = 0;
  h = 0;
}

Rect::Rect(int w, int h)
{
  this->x = 0;
  this->y = 0;
  this->w = w;
  this->h = h;
}

Rect::Rect(const Size& size)
{
  x = 0;
  y = 0;
  w = size.w;
  h = size.h;
}

Rect::Rect(const Rect& rect)
{
  x = rect.x;
  y = rect.y;
  w = rect.w;
  h = rect.h;
}

Rect::Rect(const Point& point, const Size& size)
{
  this->x = point.x;
  this->y = point.y;
  this->w = size.w;
  this->h = size.h;
}

Rect::Rect(const Point& point1, const Point& point2)
{
  Point leftTop = point1;
  Point rightBottom = point2;
  register int t;

  if (leftTop.x > rightBottom.x) {
    t = leftTop.x;
    leftTop.x = rightBottom.x;
    rightBottom.x = t;
  }

  if (leftTop.y > rightBottom.y) {
    t = leftTop.y;
    leftTop.y = rightBottom.y;
    rightBottom.y = t;
  }

  this->x = leftTop.x;
  this->y = leftTop.y;
  this->w = rightBottom.x - leftTop.x;
  this->h = rightBottom.y - leftTop.y;
}

Rect::Rect(int x, int y, int w, int h)
{
  this->x = x;
  this->y = y;
  this->w = w;
  this->h = h;
}

bool Rect::isEmpty() const
{
  return (w < 1 || h < 1);
}

Point Rect::getCenter() const
{
  return Point(x+w/2, y+h/2);
}

Point Rect::getOrigin() const
{
  return Point(x, y);
}

Point Rect::getPoint2() const
{
  return Point(x+w, y+h);
}

Size Rect::getSize() const
{
  return Size(w, h);
}

Rect& Rect::setOrigin(const Point& pt)
{
  x = pt.x;
  y = pt.y;
  return *this;
}

Rect& Rect::setSize(const Size& sz)
{
  w = sz.w;
  h = sz.h;
  return *this;
}

Rect& Rect::offset(int dx, int dy)
{
  x += dx;
  y += dy;
  return *this;
}

Rect& Rect::offset(const Point& delta)
{
  x += delta.x;
  y += delta.y;
  return *this;
}

Rect& Rect::inflate(int dw, int dh)
{
  w += dw;
  h += dh;
  return *this;
}

Rect& Rect::inflate(const Size& delta)
{
  w += delta.w;
  h += delta.h;
  return *this;
}

Rect& Rect::enlarge(int unit)
{
  x -= unit;
  y -= unit;
  w += unit<<1;
  h += unit<<1;
  return *this;
}

Rect& Rect::shrink(int unit)
{
  x += unit;
  y += unit;
  w -= unit<<1;
  h -= unit<<1;
  return *this;
}

bool Rect::contains(const Point& pt) const
{
  return
    pt.x >= x && pt.x < x+w &&
    pt.y >= y && pt.y < y+h;
}

bool Rect::contains(const Rect& rc) const
{
  if (isEmpty() || rc.isEmpty())
    return false;

  return
    rc.x >= x && rc.x+rc.w <= x+w &&
    rc.y >= y && rc.y+rc.h <= y+h;
}

bool Rect::intersects(const Rect& rc) const
{
  if (isEmpty() || rc.isEmpty())
    return false;

  return
    rc.x <= x+w && rc.x+rc.w > x &&
    rc.y <= y+h && rc.y+rc.h > y;
}

Rect Rect::createUnion(const Rect& rc) const
{
  if (isEmpty())
    return rc;
  else if (rc.isEmpty())
    return *this;
  else
    return Rect(Point(x < rc.x ? x: rc.x,
		      y < rc.y ? y: rc.y),
		Point(x+w > rc.x+rc.w ? x+w: rc.x+rc.w,
		      y+h > rc.y+rc.h ? y+h: rc.y+rc.h));
}

Rect Rect::createIntersect(const Rect& rc) const
{
  if (intersects(rc))
    return Rect(Point(x > rc.x ? x: rc.x,
		      y > rc.y ? y: rc.y),
		Point(x+w < rc.x+rc.w ? x+w: rc.x+rc.w,
		      y+h < rc.y+rc.h ? y+h: rc.y+rc.h));
  else
    return Rect();
}

bool Rect::operator==(const Rect& rc) const
{
  return
    x == rc.x && w == rc.w &&
    y == rc.y && h == rc.h;
}

bool Rect::operator!=(const Rect& rc) const
{
  return
    x != rc.x || w != rc.w ||
    y != rc.y || h != rc.h;
}

#ifdef WIN32

Rect::Rect(const tagRECT* rc)
{
  x = rc->left;
  y = rc->top;
  w = rc->right - rc->left;
  h = rc->bottom - rc->top;
}

Rect::operator tagRECT() const
{
  tagRECT rc;
  rc.left = x;
  rc.top = y;
  rc.right = x+w;
  rc.bottom = y+h;
  return rc;
}

#endif
