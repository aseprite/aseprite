// Aseprite Document Library
// Copyright (c) 2019 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_SHRINK_BOUNDS_H_INCLUDED
#define DOC_ALGORITHM_SHRINK_BOUNDS_H_INCLUDED
#pragma once

#include "doc/algorithm/flip_type.h"
#include "doc/color.h"
#include "gfx/fwd.h"

namespace doc {
class Cel;
class Image;
class Layer;

namespace algorithm {

// The layer is used in case the image is a tilemap and we need its tileset.
bool shrink_bounds(const Image* image,
                   const color_t refpixel,
                   const Layer* layer,
                   const gfx::Rect& startBounds,
                   gfx::Rect& bounds);

bool shrink_bounds(const Image* image,
                   const color_t refpixel,
                   const Layer* layer,
                   gfx::Rect& bounds);

bool shrink_cel_bounds(const Cel* cel, const color_t refpixel, gfx::Rect& bounds);

bool shrink_bounds2(const Image* a, const Image* b, const gfx::Rect& startBounds, gfx::Rect& bounds);

} // namespace algorithm
} // namespace doc

#endif
