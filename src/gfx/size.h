// ASE gfx library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GFX_SIZE_H_INCLUDED
#define GFX_SIZE_H_INCLUDED

#ifdef WIN32
  struct tagSIZE;
#endif

namespace gfx {

class Point;

class Size			// A 2D size.
{
public:

  int w, h;

  Size();
  Size(int w, int h);
  Size(const Size& size);
  explicit Size(const Point& point);

  Size createUnion(const Size& sz) const;
  Size createIntersect(const Size& sz) const;

  const Size& operator=(const Size& sz);
  const Size& operator+=(const Size& sz);
  const Size& operator-=(const Size& sz);
  const Size& operator+=(int value);
  const Size& operator-=(int value);
  const Size& operator*=(int value);
  const Size& operator/=(int value);
  Size operator+(const Size& sz) const;
  Size operator-(const Size& sz) const;
  Size operator+(int value) const;
  Size operator-(int value) const;
  Size operator*(int value) const;
  Size operator/(int value) const;
  Size operator-() const;

  bool operator==(const Size& sz) const;
  bool operator!=(const Size& sz) const;

#ifdef WIN32
  explicit Size(const tagSIZE* sz);
  operator tagSIZE() const;
#endif

};

} // namespace gfx

#endif
