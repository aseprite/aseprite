// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2009 David Capello
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.
// * Neither the name of the author nor the names of its contributors
//   may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Vaca/Point.h"
#include "Vaca/Size.h"

using namespace Vaca;

Point::Point()
{
  x = 0;
  y = 0;
}

Point::Point(int x, int y)
{
  this->x = x;
  this->y = y;
}

Point::Point(const Point& point)
{
  x = point.x;
  y = point.y;
}

Point::Point(const Size& size)
{
  x = size.w;
  y = size.h;
}

const Point& Point::operator=(const Point& pt)
{
  x = pt.x;
  y = pt.y;
  return *this;
}

const Point& Point::operator+=(const Point& pt)
{
  x += pt.x;
  y += pt.y;
  return *this;
}

const Point& Point::operator-=(const Point& pt)
{
  x -= pt.x;
  y -= pt.y;
  return *this;
}

const Point& Point::operator+=(int value)
{
  x += value;
  y += value;
  return *this;
}

const Point& Point::operator-=(int value)
{
  x -= value;
  y -= value;
  return *this;
}

const Point& Point::operator*=(int value)
{
  x *= value;
  y *= value;
  return *this;
}

const Point& Point::operator/=(int value)
{
  x /= value;
  y /= value;
  return *this;
}

Point Point::operator+(const Point& pt) const
{
  return Point(x+pt.x, y+pt.y);
}

Point Point::operator-(const Point& pt) const
{
  return Point(x-pt.x, y-pt.y);
}

Point Point::operator+(int value) const
{
  return Point(x+value, y+value);
}

Point Point::operator-(int value) const
{
  return Point(x-value, y-value);
}

Point Point::operator*(int value) const
{
  return Point(x*value, y*value);
}

Point Point::operator/(int value) const
{
  return Point(x/value, y/value);
}

Point Point::operator-() const
{
  return Point(-x, -y);
}

bool Point::operator==(const Point& pt) const
{
  return x == pt.x && y == pt.y;
}

bool Point::operator!=(const Point& pt) const
{
  return x != pt.x || y != pt.y;
}

#ifdef VACA_WINDOWS

Point::Point(CONST LPPOINT pt)
{
  x = pt->x;
  y = pt->y;
}

Point::Point(CONST LPPOINTS pt)
{
  x = pt->x;
  y = pt->y;
}

Point::operator POINT() const
{
  POINT pt;
  pt.x = x;
  pt.y = y;
  return pt;
}

#endif
