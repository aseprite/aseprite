// Aseprite
// Copyright (c) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_RESIZE_CEL_IMAGE_H_INCLUDED
#define APP_UTIL_RESIZE_CEL_IMAGE_H_INCLUDED
#pragma once

#include "doc/algorithm/resize_image.h"
#include "doc/color.h"
#include "gfx/point.h"
#include "gfx/size.h"

namespace doc {
  class Cel;
  class Image;
  class Palette;
  class RgbMap;
}

namespace app {
  class Tx;

  doc::Image* resize_image(
    doc::Image* image,
    const gfx::SizeF& scale,
    const doc::algorithm::ResizeMethod method,
    const doc::Palette* pal,
    const doc::RgbMap* rgbmap);

  void resize_cel_image(
    Tx& tx, doc::Cel* cel,
    const gfx::SizeF& scale,
    const doc::algorithm::ResizeMethod method,
    const gfx::PointF& pivot);

} // namespace app

#endif
