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

#include "raster/algorithm/flip_image.h"

#include "base/unique_ptr.h"
#include "gfx/rect.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/primitives.h"

#include <vector>

namespace raster {
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
  gfx::Rect bounds = mask->getBounds();

  switch (flipType) {

    case FlipHorizontal: {
      base::UniquePtr<Image> originalRow(Image::create(image->getPixelFormat(), bounds.w, 1));

      for (int y=bounds.y; y<bounds.y+bounds.h; ++y) {
        // Copy the current row.
        copy_image(originalRow, image, -bounds.x, -y);

        int u = bounds.x+bounds.w-1;
        for (int x=bounds.x; x<bounds.x+bounds.w; ++x, --u) {
          if (mask->containsPoint(x, y)) {
            put_pixel(image, u, y, get_pixel(originalRow, x-bounds.x, 0));
            if (!mask->containsPoint(u, y))
              put_pixel(image, x, y, bgcolor);
          }
        }
      }
      break;
    }

    case FlipVertical: {
      base::UniquePtr<Image> originalCol(Image::create(image->getPixelFormat(), 1, bounds.h));

      for (int x=bounds.x; x<bounds.x+bounds.w; ++x) {
        // Copy the current column.
        copy_image(originalCol, image, -x, -bounds.y);

        int v = bounds.y+bounds.h-1;
        for (int y=bounds.y; y<bounds.y+bounds.h; ++y, --v) {
          if (mask->containsPoint(x, y)) {
            put_pixel(image, x, v, get_pixel(originalCol, 0, y-bounds.y));
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
} // namespace raster
