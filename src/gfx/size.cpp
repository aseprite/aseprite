// ASE gfx library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "gfx/size.h"
#include "gfx/point.h"

#include <algorithm>

#ifdef WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #undef max
  #undef min
#endif

using namespace gfx;

Size::Size()
{
  w = 0;
  h = 0;
}

Size::Size(int w, int h)
{
  this->w = w;
  this->h = h;
}

Size::Size(const Size& size)
{
  w = size.w;
  h = size.h;
}

Size::Size(const Point& point)
{
  w = point.x;
  h = point.y;
}

Size Size::createUnion(const Size& sz) const
{
  return Size(std::max(w, sz.w),
	      std::max(h, sz.h));
}

Size Size::createIntersect(const Size& sz) const
{
  return Size(std::min(w, sz.w),
	      std::min(h, sz.h));
}

const Size& Size::operator=(const Size& sz)
{
  w = sz.w;
  h = sz.h;
  return *this;
}

const Size& Size::operator+=(const Size& sz)
{
  w += sz.w;
  h += sz.h;
  return *this;
}

const Size& Size::operator-=(const Size& sz)
{
  w -= sz.w;
  h -= sz.h;
  return *this;
}

const Size& Size::operator+=(int value)
{
  w += value;
  h += value;
  return *this;
}

const Size& Size::operator-=(int value)
{
  w -= value;
  h -= value;
  return *this;
}

const Size& Size::operator*=(int value)
{
  w *= value;
  h *= value;
  return *this;
}

const Size& Size::operator/=(int value)
{
  w /= value;
  h /= value;
  return *this;
}

Size Size::operator+(const Size& sz) const
{
  return Size(w+sz.w, h+sz.h);
}

Size Size::operator-(const Size& sz) const
{
  return Size(w-sz.w, h-sz.h);
}

Size Size::operator+(int value) const
{
  return Size(w+value, h+value);
}

Size Size::operator-(int value) const
{
  return Size(w-value, h-value);
}

Size Size::operator*(int value) const
{
  return Size(w*value, h*value);
}

Size Size::operator/(int value) const
{
  return Size(w/value, h/value);
}

Size Size::operator-() const
{
  return Size(-w, -h);
}

bool Size::operator==(const Size& sz) const
{
  return w == sz.w && h == sz.h;
}

bool Size::operator!=(const Size& sz) const
{
  return w != sz.w || h != sz.h;
}

#ifdef WIN32

Size::Size(const tagSIZE* sz)
{
  w = sz->cx;
  h = sz->cy;
}

Size::operator tagSIZE() const
{
  tagSIZE sz;
  sz.cx = w;
  sz.cy = h;
  return sz;
}

#endif
