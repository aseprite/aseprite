// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algorithm/shrink_bounds.h"

#include "doc/image.h"

namespace doc {
namespace algorithm {

static bool is_same_pixel(PixelFormat pixelFormat, color_t pixel1, color_t pixel2)
{
  switch (pixelFormat) {
    case IMAGE_RGB:
      if (rgba_geta(pixel1) == 0 && rgba_geta(pixel2) == 0)
        return true;
      break;
    case IMAGE_GRAYSCALE:
      if (graya_geta(pixel1) == 0 && graya_geta(pixel2) == 0)
        return true;
      break;
  }
  return pixel1 == pixel2;
}

bool shrink_bounds(Image *image, gfx::Rect& bounds, color_t refpixel)
{
  bool shrink;
  int u, v;

  bounds = image->bounds();

  // Shrink left side
  for (u=bounds.x; u<bounds.x+bounds.w; ++u) {
    shrink = true;
    for (v=bounds.y; v<bounds.y+bounds.h; ++v) {
      if (!is_same_pixel(image->pixelFormat(), image->getPixel(u, v), refpixel)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    ++bounds.x;
    --bounds.w;
  }

  // Shrink right side
  for (u=bounds.x+bounds.w-1; u>=bounds.x; --u) {
    shrink = true;
    for (v=bounds.y; v<bounds.y+bounds.h; ++v) {
      if (!is_same_pixel(image->pixelFormat(), image->getPixel(u, v), refpixel)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    --bounds.w;
  }

  // Shrink top side
  for (v=bounds.y; v<bounds.y+bounds.h; ++v) {
    shrink = true;
    for (u=bounds.x; u<bounds.x+bounds.w; ++u) {
      if (!is_same_pixel(image->pixelFormat(), image->getPixel(u, v), refpixel)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    ++bounds.y;
    --bounds.h;
  }

  // Shrink bottom side
  for (v=bounds.y+bounds.h-1; v>=bounds.y; --v) {
    shrink = true;
    for (u=bounds.x; u<bounds.x+bounds.w; ++u) {
      if (!is_same_pixel(image->pixelFormat(), image->getPixel(u, v), refpixel)) {
        shrink = false;
        break;
      }
    }
    if (!shrink)
      break;
    --bounds.h;
  }

  return (!bounds.isEmpty());
}

} // namespace algorithm
} // namespace doc
