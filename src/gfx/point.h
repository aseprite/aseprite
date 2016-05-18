// Aseprite Gfx Library
// Copyright (C) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_POINT_H_INCLUDED
#define GFX_POINT_H_INCLUDED
#pragma once

namespace gfx {

template<typename T>
class SizeT;

// A 2D coordinate in the screen.
template<typename T>
class PointT
{
public:
  T x, y;

  PointT() : x(0), y(0) {
  }

  PointT(const T& x, const T& y) : x(x), y(y) {
  }

  PointT(const PointT& point) : x(point.x), y(point.y) {
  }

  template<typename T2>
  explicit PointT(const PointT<T2>& point) : x(static_cast<T>(point.x)),
                                             y(static_cast<T>(point.y)) {
  }

  explicit PointT(const SizeT<T>& size) : x(size.w), y(size.h) {
  }

  const PointT& operator=(const PointT& pt) {
    x = pt.x;
    y = pt.y;
    return *this;
  }

  const PointT& operator+=(const PointT& pt) {
    x += pt.x;
    y += pt.y;
    return *this;
  }

  const PointT& operator-=(const PointT& pt) {
    x -= pt.x;
    y -= pt.y;
    return *this;
  }

  const PointT& operator+=(const T& value) {
    x += value;
    y += value;
    return *this;
  }

  const PointT& operator-=(const T& value) {
    x -= value;
    y -= value;
    return *this;
  }

  const PointT& operator*=(const T& value) {
    x *= value;
    y *= value;
    return *this;
  }

  const PointT& operator/=(const T& value) {
    x /= value;
    y /= value;
    return *this;
  }

  PointT operator+(const PointT& pt) const {
    return PointT(x+pt.x, y+pt.y);
  }

  PointT operator-(const PointT& pt) const {
    return PointT(x-pt.x, y-pt.y);
  }

  PointT operator+(const T& value) const {
    return PointT(x+value, y+value);
  }

  PointT operator-(const T& value) const {
    return PointT(x-value, y-value);
  }

  PointT operator*(const T& value) const {
    return PointT(x*value, y*value);
  }

  PointT operator/(const T& value) const {
    return PointT(x/value, y/value);
  }

  PointT operator-() const {
    return PointT(-x, -y);
  }

  bool operator==(const PointT& pt) const {
    return x == pt.x && y == pt.y;
  }

  bool operator!=(const PointT& pt) const {
    return x != pt.x || y != pt.y;
  }

};

typedef PointT<int> Point;
typedef PointT<double> PointF;

} // namespace gfx

#endif
