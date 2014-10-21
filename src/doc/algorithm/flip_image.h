// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_FLIP_IMAGE_H_INCLUDED
#define DOC_ALGORITHM_FLIP_IMAGE_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "doc/algorithm/flip_type.h"

namespace doc {
  class Image;
  class Mask;

  namespace algorithm {

    // Flips the rectangular region specified in the "bounds" parameter.
    void flip_image(Image* image, const gfx::Rect& bounds, FlipType flipType);

    // Flips an irregular region specified by the "mask". The
    // "bgcolor" is used to clear areas that aren't covered by a
    // mirrored pixel.
    void flip_image_with_mask(Image* image, const Mask* mask, FlipType flipType, int bgcolor);

  }
}

#endif
