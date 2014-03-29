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

#ifndef RASTER_PRIMITIVES_FAST_H_INCLUDED
#define RASTER_PRIMITIVES_FAST_H_INCLUDED
#pragma once

#include "raster/color.h"
#include "raster/image_impl.h"

namespace raster {
  class Image;

  template<class Traits>
  inline typename Traits::pixel_t get_pixel_fast(const Image* image, int x, int y) {
    ASSERT(x >= 0 && x < image->getWidth());
    ASSERT(y >= 0 && y < image->getHeight());

    return *(((ImageImpl<Traits>*)image)->address(x, y));
  }

  template<class Traits>
  inline void put_pixel_fast(Image* image, int x, int y, typename Traits::pixel_t color) {
    ASSERT(x >= 0 && x < image->getWidth());
    ASSERT(y >= 0 && y < image->getHeight());

    *(((ImageImpl<Traits>*)image)->address(x, y)) = color;
  }

  //////////////////////////////////////////////////////////////////////
  // Bitmap specialization

  template<>
  inline BitmapTraits::pixel_t get_pixel_fast<BitmapTraits>(const Image* image, int x, int y) {
    ASSERT(x >= 0 && x < image->getWidth());
    ASSERT(y >= 0 && y < image->getHeight());

    return (*image->getPixelAddress(x, y)) & (1 << (x % 8)) ? 1: 0;
  }

  template<>
  inline void put_pixel_fast<BitmapTraits>(Image* image, int x, int y, BitmapTraits::pixel_t color) {
    ASSERT(x >= 0 && x < image->getWidth());
    ASSERT(y >= 0 && y < image->getHeight());

    if (color)
      *image->getPixelAddress(x, y) |= (1 << (x % 8));
    else
      *image->getPixelAddress(x, y) &= ~(1 << (x % 8));
  }

} // namespace raster

#endif
