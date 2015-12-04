// Aseprite Gfx Library
// Copyright (C) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_BORDER_H_INCLUDED
#define GFX_BORDER_H_INCLUDED
#pragma once

namespace gfx {

template<typename T>
class SizeT;

template<typename T>
class BorderT
{
public:
  BorderT() :
    m_left(0),
    m_top(0),
    m_right(0),
    m_bottom(0) {
  }

  BorderT(const T& left, const T& top, const T& right, const T& bottom) :
    m_left(left),
    m_top(top),
    m_right(right),
    m_bottom(bottom) {
  }

  explicit BorderT(const T& allSides) :
    m_left(allSides),
    m_top(allSides),
    m_right(allSides),
    m_bottom(allSides) {
  }

  T left() const { return m_left; };
  T top() const { return m_top; };
  T right() const { return m_right; };
  T bottom() const { return m_bottom; };

  T width() const { return m_left + m_right; };
  T height() const { return m_top + m_bottom; };

  void left(const T& left) { m_left = left; }
  void top(const T& top) { m_top = top; }
  void right(const T& right) { m_right = right; }
  void bottom(const T& bottom) { m_bottom = bottom; }

  SizeT<T> size() const {
    return SizeT<T>(m_left + m_right, m_top + m_bottom);
  }

  const BorderT& operator+=(const BorderT& br) {
    m_left += br.m_left;
    m_top += br.m_top;
    m_right += br.m_right;
    m_bottom += br.m_bottom;
    return *this;
  }

  const BorderT& operator-=(const BorderT& br) {
    m_left -= br.m_left;
    m_top -= br.m_top;
    m_right -= br.m_right;
    m_bottom -= br.m_bottom;
    return *this;
  }

  const BorderT& operator*=(const BorderT& br) {
    m_left *= br.m_left;
    m_top *= br.m_top;
    m_right *= br.m_right;
    m_bottom *= br.m_bottom;
    return *this;
  }

  const BorderT& operator/=(const BorderT& br) {
    m_left /= br.m_left;
    m_top /= br.m_top;
    m_right /= br.m_right;
    m_bottom /= br.m_bottom;
    return *this;
  }

  const BorderT& operator+=(const T& value) {
    m_left += value;
    m_top += value;
    m_right += value;
    m_bottom += value;
    return *this;
  }

  const BorderT& operator-=(const T& value) {
    m_left -= value;
    m_top -= value;
    m_right -= value;
    m_bottom -= value;
    return *this;
  }

  const BorderT& operator*=(const T& value) {
    m_left *= value;
    m_top *= value;
    m_right *= value;
    m_bottom *= value;
    return *this;
  }

  const BorderT& operator/=(const T& value) {
    m_left /= value;
    m_top /= value;
    m_right /= value;
    m_bottom /= value;
    return *this;
  }

  BorderT operator+(const BorderT& br) const {
    return BorderT(m_left + br.left(),
                   m_top + br.top(),
                   m_right + br.right(),
                   m_bottom + br.bottom());
  }

  BorderT operator-(const BorderT& br) const {
    return BorderT(m_left - br.left(),
                   m_top - br.top(),
                   m_right - br.right(),
                   m_bottom - br.bottom());
  }

  BorderT operator*(const BorderT& br) const {
    return BorderT(m_left * br.left(),
                   m_top * br.top(),
                   m_right * br.right(),
                   m_bottom * br.bottom());
  }

  BorderT operator/(const BorderT& br) const {
    return BorderT(m_left / br.left(),
                   m_top / br.top(),
                   m_right / br.right(),
                   m_bottom / br.bottom());
  }

  BorderT operator+(const T& value) const {
    return BorderT(m_left + value,
                   m_top + value,
                   m_right + value,
                   m_bottom + value);
  }

  BorderT operator-(const T& value) const {
    return BorderT(m_left - value,
                   m_top - value,
                   m_right - value,
                   m_bottom - value);
  }

  BorderT operator*(const T& value) const {
    return BorderT(m_left * value,
                   m_top * value,
                   m_right * value,
                   m_bottom * value);
  }

  BorderT operator/(const T& value) const {
    return BorderT(m_left / value,
                   m_top / value,
                   m_right / value,
                   m_bottom / value);
  }

  BorderT operator-() const {
    return BorderT(-m_left, -m_top, -m_right, -m_bottom);
  }

  bool operator==(const BorderT& br) const {
    return
      m_left == br.m_left && m_top == br.m_top &&
      m_right == br.m_right && m_bottom == br.m_bottom;
  }

  bool operator!=(const BorderT& br) const {
    return
      m_left != br.m_left || m_top != br.m_top ||
      m_right != br.m_right || m_bottom != br.m_bottom;
  }

private:
  T m_left;
  T m_top;
  T m_right;
  T m_bottom;
};

typedef BorderT<int> Border;

} // namespace gfx

#endif
