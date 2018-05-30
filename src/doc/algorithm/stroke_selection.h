// Aseprite Document Library
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_STROKE_SELECTION_H_INCLUDED
#define DOC_ALGORITHM_STROKE_SELECTION_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "gfx/point.h"

namespace doc {
  class Image;
  class Mask;
  namespace algorithm {

    void stroke_selection(Image* image,
                          const gfx::Point& offset,
                          const Mask* mask,
                          const color_t color);

  } // namespace algorithm
} // namespace doc

#endif
