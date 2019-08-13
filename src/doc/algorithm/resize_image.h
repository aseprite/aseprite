// Aseprite Document Library
// Copyright (c) 2019  Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_RESIZE_IMAGE_H_INCLUDED
#define DOC_ALGORITHM_RESIZE_IMAGE_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "gfx/fwd.h"

namespace doc {
  class Image;
  class Palette;
  class RgbMap;

  namespace algorithm {

    enum ResizeMethod {
      RESIZE_METHOD_NEAREST_NEIGHBOR,
      RESIZE_METHOD_BILINEAR,
      RESIZE_METHOD_ROTSPRITE,
    };

    // Resizes the source image 'src' to the destination image 'dst'.
    //
    // Warning: If you are using the RESIZE_METHOD_BILINEAR, it is
    // recommended to use 'fixup_image_transparent_colors' function
    // over the source image 'src' BEFORE using this routine.
    void resize_image(const Image* src,
                      Image* dst,
                      const ResizeMethod method,
                      const Palette* palette,
                      const RgbMap* rgbmap,
                      const color_t maskColor);

    // It does not modify the image to the human eye, but internally
    // tries to fixup all colors that are completely transparent
    // (alpha = 0) with the average of its 4-neighbors.  Useful if you
    // want to use resize_image() with images that contains
    // transparent pixels.
    void fixup_image_transparent_colors(Image* image);

  }
}

#endif
