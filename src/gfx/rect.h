// ASE gfx library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GFX_RECT_H_INCLUDED
#define GFX_RECT_H_INCLUDED

#ifdef WIN32
  struct tagRECT;
#endif

namespace gfx {

class Point;
class Size;

class Rect			// A rectangle.
{
public:

  int x, y, w, h;

  // Creates a new empty rectangle with the origin in 0,0. 
  Rect();

  // Creates a new rectangle with the specified size with the origin in 0,0.
  Rect(int w, int h);

  // Creates a new rectangle with the specified size with the origin in 0,0.
  explicit Rect(const Size& size);

  Rect(const Rect& rect);

  Rect(const Point& point, const Size& size);

  // Creates a new rectangle with the origin in point1 and size
  // equal to point2-point1.
  //
  // If a coordinate of point1 is greater than point2, the coordinates
  // are swapped. The resulting rectangle will be:
  // 
  // x = min(point1.x, point2.x)
  // y = min(point1.y, point2.y)
  // w = max(point1.x, point2.x) - x
  // h = max(point1.x, point2.x) - y
  //
  // See that point2 isn't included in the rectangle, it's like the
  // point returned by getPoint2() member function.
  Rect(const Point& point1, const Point& point2);

  Rect(int x, int y, int w, int h);

  // Verifies if the width and/or height of the rectangle are less or
  // equal than zero.
  bool isEmpty() const;

  // Returns the middle point of the rectangle (x+w/2, y+h/2).
  Point getCenter() const;

  // Returns the point in the upper-left corner (that is inside the
  // rectangle).
  Point getOrigin() const;

  // Returns point in the lower-right corner that is outside the
  // rectangle (x+w, y+h).
  Point getPoint2() const;

  Size getSize() const;

  Rect& setOrigin(const Point& pt);
  Rect& setSize(const Size& sz);

  // Moves the rectangle origin in the specified delta.
  Rect& offset(int dx, int dy);
  Rect& offset(const Point& delta);
  Rect& inflate(int dw, int dh);
  Rect& inflate(const Size& delta);

  Rect& enlarge(int unit);
  Rect& shrink(int unit);

  // Returns true if this rectangle encloses the pt point.
  bool contains(const Point& pt) const;

  // Returns true if this rectangle entirely contains the rc rectangle.
  bool contains(const Rect& rc) const;

  // Returns true if the intersection between this rectangle with rc
  // rectangle is not empty.
  bool intersects(const Rect& rc) const;

  // Returns the union rectangle between this and rc rectangle.
  Rect createUnion(const Rect& rc) const;

  // Returns the intersection rectangle between this and rc rectangles.
  Rect createIntersect(const Rect& rc) const;

  bool operator==(const Rect& rc) const;
  bool operator!=(const Rect& rc) const;

#ifdef WIN32
  explicit Rect(const tagRECT* rc);
  operator tagRECT() const;
#endif

};

} // namespace gfx

#endif

