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

#ifndef VACA_POINT_H
#define VACA_POINT_H

#include "Vaca/base.h"

namespace Vaca {

/**
   A 2D coordinate in the screen or client area of a widget.
*/
class VACA_DLL Point
{
public:

  int x, y;

  Point();
  Point(int x, int y);
  Point(const Point& point);
  explicit Point(const Size& size);

  const Point& operator=(const Point& pt);
  const Point& operator+=(const Point& pt);
  const Point& operator-=(const Point& pt);
  const Point& operator+=(int value);
  const Point& operator-=(int value);
  const Point& operator*=(int value);
  const Point& operator/=(int value);
  Point operator+(const Point& pt) const;
  Point operator-(const Point& pt) const;
  Point operator+(int value) const;
  Point operator-(int value) const;
  Point operator*(int value) const;
  Point operator/(int value) const;
  Point operator-() const;

  bool operator==(const Point& pt) const;
  bool operator!=(const Point& pt) const;

#ifdef VACA_WINDOWS
  explicit Point(CONST LPPOINT pt);
  explicit Point(CONST LPPOINTS pt);
  operator POINT() const;
#endif

};

} // namespace Vaca

#endif // VACA_POINT_H
