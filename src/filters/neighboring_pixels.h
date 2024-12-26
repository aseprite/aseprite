// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_NEIGHBORING_PIXELS_H_INCLUDED
#define FILTERS_NEIGHBORING_PIXELS_H_INCLUDED
#pragma once

#include "doc/image.h"
#include "doc/image_traits.h"
#include "filters/tiled_mode.h"

#include <vector>

namespace filters {
using namespace doc;

// Calls the specified "delegate" for all neighboring pixels in a 2D
// (width*height) matrix located in (x,y) where its center is the
// (centerX,centerY) element of the matrix.
template<typename Traits, typename Delegate>
inline void get_neighboring_pixels(const doc::Image* sourceImage,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   int centerX,
                                   int centerY,
                                   TiledMode tiledMode,
                                   Delegate& delegate)
{
  // Y position to get pixel.
  int getx, gety = y - centerY;
  int addx, addy = 0;
  if (gety < 0) {
    if (int(tiledMode) & int(TiledMode::Y_AXIS))
      gety = sourceImage->height() - (-(gety + 1) % sourceImage->height()) - 1;
    else {
      addy = -gety;
      gety = 0;
    }
  }
  else if (gety >= sourceImage->height()) {
    if (int(tiledMode) & int(TiledMode::Y_AXIS))
      gety = gety % sourceImage->height();
    else
      gety = sourceImage->height() - 1;
  }

  for (int dy = 0; dy < height; ++dy) {
    // X position to get pixel.
    getx = x - centerX;
    addx = 0;
    if (getx < 0) {
      if (int(tiledMode) & int(TiledMode::X_AXIS))
        getx = sourceImage->width() - (-(getx + 1) % sourceImage->width()) - 1;
      else {
        addx = -getx;
        getx = 0;
      }
    }
    else if (getx >= sourceImage->width()) {
      if (int(tiledMode) & int(TiledMode::X_AXIS))
        getx = getx % sourceImage->width();
      else
        getx = sourceImage->width() - 1;
    }

    typename Traits::const_address_t srcAddress =
      reinterpret_cast<typename Traits::const_address_t>(sourceImage->getPixelAddress(getx, gety));

    for (int dx = 0; dx < width; dx++) {
      // Call the delegate for each pixel value.
      delegate(*srcAddress);

      // Update X position to get pixel.
      if (getx < sourceImage->width() - 1) {
        ++getx;
        if (addx == 0)
          ++srcAddress;
        else
          --addx;
      }
      else if (int(tiledMode) & int(TiledMode::X_AXIS)) {
        getx = 0;
        srcAddress = reinterpret_cast<typename Traits::const_address_t>(
          sourceImage->getPixelAddress(getx, gety));
      }
    }

    // Update Y position to get pixel
    if (gety < sourceImage->height() - 1) {
      if (addy == 0)
        ++gety;
      else
        --addy;
    }
    else if (int(tiledMode) & int(TiledMode::Y_AXIS))
      gety = 0;
  }
}

} // namespace filters

#endif
