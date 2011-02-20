// ASE gfx library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GFX_BORDER_H_INCLUDED
#define GFX_BORDER_H_INCLUDED

namespace gfx {

class Size;

class Border
{
public:
  Border();
  Border(int left, int top, int right, int bottom);
  explicit Border(int allSides);

  int left() const { return m_left; };
  int top() const { return m_top; };
  int right() const { return m_right; };
  int bottom() const { return m_bottom; };

  void left(int left) { m_left = left; }
  void top(int top) { m_top = top; }
  void right(int right) { m_right = right; }
  void bottom(int bottom) { m_bottom = bottom; }

  Size getSize() const;

  const Border& operator+=(const Border& br);
  const Border& operator-=(const Border& br);
  const Border& operator*=(const Border& br);
  const Border& operator/=(const Border& br);
  const Border& operator+=(int value);
  const Border& operator-=(int value);
  const Border& operator*=(int value);
  const Border& operator/=(int value);
  Border operator+(const Border& br) const;
  Border operator-(const Border& br) const;
  Border operator*(const Border& br) const;
  Border operator/(const Border& br) const;
  Border operator+(int value) const;
  Border operator-(int value) const;
  Border operator*(int value) const;
  Border operator/(int value) const;
  Border operator-() const;

  bool operator==(const Border& br) const;
  bool operator!=(const Border& br) const;

private:
  int m_left;
  int m_top;
  int m_right;
  int m_bottom;
};

} // namespace gfx

#endif

