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

#include "Vaca/Size.h"
#include "Vaca/Point.h"

using namespace Vaca;

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
  return Size(max_value(w, sz.w),
	      max_value(h, sz.h));
}

Size Size::createIntersect(const Size& sz) const
{
  return Size(min_value(w, sz.w),
	      min_value(h, sz.h));
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

#ifdef VACA_WINDOWS

Size::Size(CONST LPSIZE sz)
{
  w = sz->cx;
  h = sz->cy;
}

Size::operator SIZE() const
{
  SIZE sz;
  sz.cx = w;
  sz.cy = h;
  return sz;
}

#endif
