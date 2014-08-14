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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/algorithm/shrink_bounds.h"

#include "raster/image.h"

namespace raster {
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
} // namespace raster
