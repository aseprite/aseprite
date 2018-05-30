// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_FILL_SELECTION_H_INCLUDED
#define APP_UTIL_FILL_SELECTION_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "gfx/point.h"

namespace doc {
  class Image;
  class Mask;
}

namespace app {

  void fill_selection(doc::Image* image,
                      const gfx::Point& offset,
                      const doc::Mask* mask,
                      const doc::color_t color);

} // namespace app

#endif
