// Aseprite Document Library
// Copyright (c) 2020 Igara Studio S.A.
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_STROKE_SELECTION_H_INCLUDED
#define DOC_ALGORITHM_STROKE_SELECTION_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "gfx/rect.h"

namespace doc {
class Grid;
class Image;
class Mask;

namespace algorithm {

void stroke_selection(Image* image,
                      const gfx::Rect& imageBounds,
                      const Mask* mask,
                      const color_t color,
                      const Grid* grid = nullptr);

// 新重载，支持 width/location
void stroke_selection(Image* image,
                      const gfx::Rect& imageBounds,
                      const Mask* mask,
                      const color_t color,
                      int width,
                      const std::string& location,
                      const Grid* grid = nullptr);

} // namespace algorithm
} // namespace doc

#endif
