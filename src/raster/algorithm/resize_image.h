/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RASTER_ALGORITHM_RESIZE_IMAGE_H_INCLUDED
#define RASTER_ALGORITHM_RESIZE_IMAGE_H_INCLUDED
#pragma once

#include "gfx/fwd.h"

namespace raster {
  class Image;
  class Palette;
  class RgbMap;

  namespace algorithm {

    enum ResizeMethod {
      RESIZE_METHOD_NEAREST_NEIGHBOR,
      RESIZE_METHOD_BILINEAR,
    };

    // Resizes the source image 'src' to the destination image 'dst'.
    //
    // Warning: If you are using the RESIZE_METHOD_BILINEAR, it is
    // recommended to use 'fixup_image_transparent_colors' function
    // over the source image 'src' BEFORE using this routine.
    void resize_image(const Image* src, Image* dst, ResizeMethod method, const Palette* palette, const RgbMap* rgbmap);

    // It does not modify the image to the human eye, but internally
    // tries to fixup all colors that are completelly transparent
    // (alpha = 0) with the average of its 4-neighbors.  Useful if you
    // want to use resize_image() with images that contains
    // transparent pixels.
    void fixup_image_transparent_colors(Image* image);

  }
}

#endif
