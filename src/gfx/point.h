// ASE gfx library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GFX_POINT_H_INCLUDED
#define GFX_POINT_H_INCLUDED

#ifdef WIN32
  struct tagPOINT;
  struct tagPOINTS;
#endif

namespace gfx {

class Size;

class Point			// A 2D coordinate in the screen.
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

#ifdef WIN32
  explicit Point(const tagPOINT* pt);
  explicit Point(const tagPOINTS* pt);
  operator tagPOINT() const;
#endif

};

} // namespace gfx

#endif
