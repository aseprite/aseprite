// Aseprite
// Copyright (C) 2025 Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_MOVING_UTILS_H_INCLUDED
#define APP_UTIL_MOVING_UTILS_H_INCLUDED
#pragma once

#include "gfx/point.h"

namespace app {

enum LockedAxis {
  NONE,
  X,
  Y,
  R_DIAG,
  L_DIAG,
};

void lockAxis(LockedAxis& lockedAxis, gfx::PointF& delta);

void lockAxis(LockedAxis& lockedAxis, double& dx, double& dy);

} // namespace app

#endif // APP_UTIL_MOVING_UTILS_H_INCLUDED
