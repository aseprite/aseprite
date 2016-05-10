// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_SHRINK_BOUNDS_H_INCLUDED
#define DOC_ALGORITHM_SHRINK_BOUNDS_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "doc/algorithm/flip_type.h"
#include "doc/color.h"

namespace doc {
  class Image;

  namespace algorithm {

    bool shrink_bounds(const Image* image,
                       const gfx::Rect& start_bounds,
                       gfx::Rect& bounds,
                       color_t refpixel);

    bool shrink_bounds(const Image* image,
                       gfx::Rect& bounds,
                       color_t refpixel);

    bool shrink_bounds2(const Image* a,
                        const Image* b,
                        const gfx::Rect& start_bounds,
                        gfx::Rect& bounds);

  } // algorithm
} // doc

#endif
