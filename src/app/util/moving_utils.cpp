// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/moving_utils.h"

#include <cmath>

namespace app {

void lockAxis(LockedAxis& lockedAxis, gfx::PointF& delta)
{
  double dx = delta.x;
  double dy = delta.y;
  lockAxis(lockedAxis, dx, dy);
  delta.x = dx;
  delta.y = dy;
}

void lockAxis(LockedAxis& lockedAxis, double& dx, double& dy)
{
  if (lockedAxis == LockedAxis::NONE) {
    if (std::abs(dx) * 3 > std::abs(dy) * 7) {
      lockedAxis = LockedAxis::Y;
      dy = 0.0;
    }
    else if (std::abs(dy) * 3 > std::abs(dx) * 7) {
      lockedAxis = LockedAxis::X;
      dx = 0.0;
    }
    else {
      const bool sameSign = std::signbit(dx) == std::signbit(dy);
      lockedAxis = sameSign ? LockedAxis::R_DIAG : LockedAxis::L_DIAG;
      dy = sameSign ? dx : -dx;
    }
  }
  else {
    switch (lockedAxis) {
      case LockedAxis::X:      dx = 0.0; break;
      case LockedAxis::Y:      dy = 0.0; break;
      case LockedAxis::R_DIAG: dy = dx; break;
      case LockedAxis::L_DIAG: dy = -dx; break;
    }
  }
}

} // namespace app
