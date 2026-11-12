// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/editor/view_options.h"

namespace app {

void ViewOptions::calcMatrices(const gfx::PointF& center)
{
  matrix.setIdentity();

  if (isRotated() || xflip || yflip) {
    if (isRotated())
      matrix.setRotate(angle);

    matrix.preTranslate(-center.x, -center.y);
    if (xflip || yflip)
      matrix.postConcat(flipMatrix());
    matrix.postTranslate(center.x, center.y);
  }

  auto inv = matrix.invert();
  invMatrix = (inv ? *inv : matrix);
}

} // namespace app
