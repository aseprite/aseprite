// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algorithm/flip_image.h"

#include "gfx/rect.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/primitives.h"

#include <memory>
#include <vector>

namespace doc {
namespace algorithm {

void flip_image(Image* image, const gfx::Rect& bounds, FlipType flipType)
{
  switch (flipType) {

    case FlipHorizontal:
      for (int y=bounds.y; y<bounds.y+bounds.h; ++y) {
        int u = bounds.x+bounds.w-1;
        for (int x=bounds.x; x<bounds.x+bounds.w/2; ++x, --u) {
          uint32_t c1 = get_pixel(image, x, y);
          uint32_t c2 = get_pixel(image, u, y);
          put_pixel(image, x, y, c2);
          put_pixel(image, u, y, c1);
        }
      }
      break;

    case FlipVertical: {
      int v = bounds.y+bounds.h-1;
      for (int y=bounds.y; y<bounds.y+bounds.h/2; ++y, --v) {
        for (int x=bounds.x; x<bounds.x+bounds.w; ++x) {
          uint32_t c1 = get_pixel(image, x, y);
          uint32_t c2 = get_pixel(image, x, v);
          put_pixel(image, x, y, c2);
          put_pixel(image, x, v, c1);
        }
      }
      break;
    }
  }
}

void flip_image_with_mask(Image* image, const Mask* mask, FlipType flipType, int bgcolor)
{
  gfx::Rect bounds = mask->bounds();

  switch (flipType) {

    case FlipHorizontal: {
      std::unique_ptr<Image> originalRow(Image::create(image->pixelFormat(), bounds.w, 1));

      for (int y=bounds.y; y<bounds.y+bounds.h; ++y) {
        // Copy the current row.
        originalRow->copy(image, gfx::Clip(0, 0, bounds.x, y, bounds.w, 1));

        int u = bounds.x+bounds.w-1;
        for (int x=bounds.x; x<bounds.x+bounds.w; ++x, --u) {
          if (mask->containsPoint(x, y)) {
            put_pixel(image, u, y, get_pixel(originalRow.get(), x-bounds.x, 0));
            if (!mask->containsPoint(u, y))
              put_pixel(image, x, y, bgcolor);
          }
        }
      }
      break;
    }

    case FlipVertical: {
      std::unique_ptr<Image> originalCol(Image::create(image->pixelFormat(), 1, bounds.h));

      for (int x=bounds.x; x<bounds.x+bounds.w; ++x) {
        // Copy the current column.
        originalCol->copy(image, gfx::Clip(0, 0, x, bounds.y, 1, bounds.h));

        int v = bounds.y+bounds.h-1;
        for (int y=bounds.y; y<bounds.y+bounds.h; ++y, --v) {
          if (mask->containsPoint(x, y)) {
            put_pixel(image, x, v, get_pixel(originalCol.get(), 0, y-bounds.y));
            if (!mask->containsPoint(x, v))
              put_pixel(image, x, y, bgcolor);
          }
        }
      }
      break;
    }

  }
}

} // namespace algorithm
} // namespace doc
