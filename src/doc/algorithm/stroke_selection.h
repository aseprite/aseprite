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
                      // This can be a color_t or a tile_t if the image is a tilemap
                      const color_t color,
                      // Optional grid for tilemaps
                      const Grid* grid = nullptr);

} // namespace algorithm
} // namespace doc

#endif
