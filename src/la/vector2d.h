// ASEPRITE Linear Algebra Library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef LA_VECTOR2D_H_INCLUDED
#define LA_VECTOR2D_H_INCLUDED

#include <cmath>

namespace la {

template<typename T>
class Vector2d {
public:
  T x, y;

  Vector2d() {
    x = y = 0.0;
  }

  Vector2d(const T& u, const T& v) {
    x = u; y = v;
  }

  Vector2d(const Vector2d& v) {
    x = v.x; y = v.y;
  }

  Vector2d operator-() const { return Vector2d(-x, -y); }

  Vector2d operator+(const Vector2d& v) const { return Vector2d(x+v.x, y+v.y); }
  Vector2d operator-(const Vector2d& v) const { return Vector2d(x-v.x, y-v.y); }
  T        operator*(const Vector2d& v) const { return dotProduct(v); }
  Vector2d operator*(const T&        f) const { return Vector2d(x*f, y*f); }
  Vector2d operator/(const T&        f) const { return Vector2d(x/f, y/f); }

  Vector2d& operator= (const Vector2d& v) { x=v.x; y=v.y; return *this; }
  Vector2d& operator+=(const Vector2d& v) { x+=v.x; y+=v.y; return *this; }
  Vector2d& operator-=(const Vector2d& v) { x-=v.x; y-=v.y; return *this; }
  Vector2d& operator*=(const T&        f) { x*=f; y*=f; return *this; }
  Vector2d& operator/=(const T&        f) { x/=f; y/=f; return *this; }

  T magnitude() const {
    return std::sqrt(x*x + y*y);
  }

  T dotProduct(const Vector2d& v) const {
    return x*v.x + y*v.y;
  }

  Vector2d projectOn(const Vector2d& v) const {
    return v * (this->dotProduct(v) / std::pow(v.magnitude(), 2));
  }

  T angle() const {
    return std::atan2(y, x);
  }

  Vector2d normalize() const {
    return Vector2d(*this) / magnitude();
  }

  Vector2d getNormal() const {
    return Vector2d(y, -x);
  }

};

} // namespace la

template<typename T>
la::Vector2d<T> operator*(const T& f, const la::Vector2d<T>& v) {
  return la::Vector2d<T>(v.x*f, v.y*f);
}

#endif // LA_VECTOR2D_H_INCLUDED
