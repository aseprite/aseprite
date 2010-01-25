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

#ifndef VACA_RECT_H
#define VACA_RECT_H

#include "Vaca/base.h"

namespace Vaca {

/**
   A rectangle.
*/
class VACA_DLL Rect
{
public:

  /**
     Horizontal position of the rectangle's upper-left corner (increase from left to right).

     @c x=0 means the beginning of the left side in the screen or
     @link Widget#getClientBounds Widget's client area@endlink.
  */
  int x;

  /**
     Vertical position of the rectangle's upper-left corner (increase from top to bottom).

     @c y=0 means the beginning of the top side in the screen or
     @link Widget#getClientBounds Widget's client area@endlink.
  */
  int y;

  /**
     Width of the rectangle (increase from left to right).

     @c w<1 means an empty rectangle.
  */
  int w;

  /**
     Height of the rectangle (increase from left to right).

     @c h<1 means an empty rectangle.
  */
  int h;

  Rect();
  Rect(int w, int h);
  explicit Rect(const Size& size);
  Rect(const Rect& rect);
  Rect(const Point& point, const Size& size);
  Rect(const Point& point1, const Point& point2);
  Rect(int x, int y, int w, int h);

  bool isEmpty() const;

  Point getCenter() const;
  Point getOrigin() const;
  Point getPoint2() const;
  Size getSize() const;

  Rect& setOrigin(const Point& pt);
  Rect& setSize(const Size& sz);

  Rect& offset(int dx, int dy);
  Rect& offset(const Point& point);
  Rect& inflate(int dw, int dh);
  Rect& inflate(const Size& size);

  Rect& enlarge(int unit);
  Rect& shrink(int unit);

  bool contains(const Point& pt) const;
  bool contains(const Rect& rc) const;
  bool intersects(const Rect& rc) const;

  Rect createUnion(const Rect& rc) const;
  Rect createIntersect(const Rect& rc) const;

  bool operator==(const Rect& rc) const;
  bool operator!=(const Rect& rc) const;

#ifdef VACA_WINDOWS
  explicit Rect(LPCRECT rc);
  operator RECT() const;
#endif

};

} // namespace Vaca

#endif // VACA_RECT_H

