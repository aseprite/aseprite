// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_AUTOCROP_H_INCLUDED
#define APP_UTIL_AUTOCROP_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "gfx/rect.h"

namespace doc {
  class Image;
  class Sprite;
}

namespace app {

  bool get_shrink_rect(int* x1, int* y1, int* x2, int* y2,
                       doc::Image *image, doc::color_t refpixel);
  bool get_shrink_rect2(int* x1, int* y1, int* x2, int* y2,
                        doc::Image* image, doc::Image* regimage);

  // Returns false if a refColor cannot be automatically decided (so
  // in this case we should ask to the user to select a specific
  // "refColor" to trim).
  bool get_best_refcolor_for_trimming(
    doc::Image* image,
    doc::color_t& refColor);

  gfx::Rect get_trimmed_bounds(
    const doc::Sprite* sprite,
    const bool byGrid);

} // namespace app

#endif
