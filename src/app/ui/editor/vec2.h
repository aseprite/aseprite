// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_VEC2_H_INCLUDED
#define APP_UI_EDITOR_VEC2_H_INCLUDED
#pragma once

#include "base/vector2d.h"
#include "gfx/point.h"

namespace app {

using vec2 = base::Vector2d<double>;

template<typename T>
inline const vec2 to_vec2(const gfx::PointT<T>& pt)
{
  return vec2(pt.x, pt.y);
}

inline const gfx::PointF to_point(const vec2& v)
{
  return gfx::PointF(v.x, v.y);
}

} // namespace app

#endif
