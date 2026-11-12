// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_VIEW_OPTIONS_H_INCLUDED
#define APP_UI_EDITOR_VIEW_OPTIONS_H_INCLUDED
#pragma once

#include "gfx/matrix.h"
#include "gfx/point.h"

namespace app {

struct ViewOptions {
  double angle = 0.0;
  bool xflip = false;
  bool yflip = false;

  gfx::Matrix matrix;
  gfx::Matrix invMatrix;

  bool isRotated() const { return angle != 0.0; }

  bool operator!=(const ViewOptions& o) const { return !operator==(o); }
  bool operator==(const ViewOptions& o) const
  {
    return (angle == o.angle && xflip == o.xflip && yflip == o.yflip);
  }

  gfx::Matrix flipMatrix() const { return gfx::Matrix::MakeScale(xflip ? -1 : 1, yflip ? -1 : 1); }

  void calcMatrices(const gfx::PointF& center);
};

} // namespace app

#endif
