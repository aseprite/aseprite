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

#ifndef FILTERS_NEIGHBORING_PIXELS_H_INCLUDED
#define FILTERS_NEIGHBORING_PIXELS_H_INCLUDED
#pragma once

#include "filters/tiled_mode.h"
#include "raster/image.h"
#include "raster/image_traits.h"

#include <vector>

namespace filters {
  using namespace raster;

  // Calls the specified "delegate" for all neighboring pixels in a 2D
  // (width*height) matrix located in (x,y) where its center is the
  // (centerX,centerY) element of the matrix.
  template<typename Traits, typename Delegate>
  inline void get_neighboring_pixels(const raster::Image* sourceImage, int x, int y,
                                     int width, int height,
                                     int centerX, int centerY,
                                     TiledMode tiledMode,
                                     Delegate& delegate)
  {
    int dx, dy;

    // Y position to get pixel.
    int getx, gety = y - centerY;
    int addx, addy = 0;
    if (gety < 0) {
      if (tiledMode & TILED_Y_AXIS)
        gety = sourceImage->getHeight() - (-(gety+1) % sourceImage->getHeight()) - 1;
      else {
        addy = -gety;
        gety = 0;
      }
    }
    else if (gety >= sourceImage->getHeight()) {
      if (tiledMode & TILED_Y_AXIS)
        gety = gety % sourceImage->getHeight();
      else
        gety = sourceImage->getHeight()-1;
    }

    for (dy=0; dy<height; ++dy) {
      // X position to get pixel.
      getx = x - centerX;
      addx = 0;
      if (getx < 0) {
        if (tiledMode & TILED_X_AXIS)
          getx = sourceImage->getWidth() - (-(getx+1) % sourceImage->getWidth()) - 1;
        else {
          addx = -getx;
          getx = 0;
        }
      }
      else if (getx >= sourceImage->getWidth()) {
        if (tiledMode & TILED_X_AXIS)
          getx = getx % sourceImage->getWidth();
        else
          getx = sourceImage->getWidth()-1;
      }

      typename Traits::const_address_t srcAddress =
        reinterpret_cast<typename Traits::const_address_t>(sourceImage->getPixelAddress(getx, gety));

      for (dx=0; dx<width; dx++) {
        // Call the delegate for each pixel value.
        delegate(*srcAddress);

        // Update X position to get pixel.
        if (getx < sourceImage->getWidth()-1) {
          ++getx;
          if (addx == 0)
            ++srcAddress;
          else
            --addx;
        }
        else if (tiledMode & TILED_X_AXIS) {
          getx = 0;
          srcAddress =
            reinterpret_cast<typename Traits::const_address_t>(sourceImage->getPixelAddress(getx, gety));
        }
      }

      // Update Y position to get pixel
      if (gety < sourceImage->getHeight()-1) {
        if (addy == 0)
          ++gety;
        else
          --addy;
      }
      else if (tiledMode & TILED_Y_AXIS)
        gety = 0;
    }
  }

} // namespace filters

#endif
