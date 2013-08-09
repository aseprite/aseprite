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
          uint32_t c1 = image_getpixel(image, x, y);
          uint32_t c2 = image_getpixel(image, u, y);
          image_putpixel(image, x, y, c2);
          image_putpixel(image, u, y, c1);
        }
      }
      break;

    case FlipVertical: {
      int section_size = image_line_size(image, bounds.w);
      std::vector<uint8_t> tmpline(section_size);

      int v = bounds.y+bounds.h-1;
      for (int y=bounds.y; y<bounds.y+bounds.h/2; ++y, --v) {
        uint8_t* address1 = static_cast<uint8_t*>(image_address(image, bounds.x, y));
        uint8_t* address2 = static_cast<uint8_t*>(image_address(image, bounds.x, v));

        // Swap lines.
        std::copy(address1, address1+section_size, tmpline.begin());
        std::copy(address2, address2+section_size, address1);
        std::copy(tmpline.begin(), tmpline.end(), address2);
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
      base::UniquePtr<Image> originalRow(Image::create(image->getPixelFormat(), mask->getBounds().w, 1));

      for (int y=bounds.y; y<bounds.y+bounds.h; ++y) {
        // Copy the current row.
        image_copy(originalRow, image, -bounds.x, -y);

        int u = bounds.x+bounds.w-1;
        for (int x=bounds.x; x<bounds.x+bounds.w; ++x, --u) {
          if (mask->containsPoint(x, y)) {
            image_putpixel(image, u, y, image_getpixel(originalRow, x-bounds.x, 0));
            if (!mask->containsPoint(u, y))
              image_putpixel(image, x, y, bgcolor);
          }
        }
      }
      break;
    }

    case FlipVertical:{
      base::UniquePtr<Image> originalCol(Image::create(image->getPixelFormat(), 1, mask->getBounds().h));

      for (int x=bounds.x; x<bounds.x+bounds.w; ++x) {
        // Copy the current column.
        image_copy(originalCol, image, -x, -bounds.y);

        int v = bounds.y+bounds.h-1;
        for (int y=bounds.y; y<bounds.y+bounds.h; ++y, --v) {
          if (mask->containsPoint(x, y)) {
            image_putpixel(image, x, v, image_getpixel(originalCol, 0, y-bounds.y));
            if (!mask->containsPoint(x, v))
              image_putpixel(image, x, y, bgcolor);
          }
        }
      }
      break;
    }

  }
}

} // namespace algorithm
} // namespace raster
