/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "raster/algorithm/flip_image.h"

#include "base/unique_ptr.h"
#include "gfx/rect.h"
#include "raster/image.h"

namespace raster { namespace algorithm {

void flip_image(Image* image, const gfx::Rect& bounds, FlipType flipType)
{
  UniquePtr<Image> area(image_crop(image, bounds.x, bounds.y, bounds.w, bounds.h, 0));
  int x2 = bounds.x+bounds.w-1;
  int y2 = bounds.y+bounds.h-1;
  int x, y;

  for (y=0; y<bounds.h; ++y)
    for (x=0; x<bounds.w; ++x)
      image_putpixel(image,
                     (flipType == FlipHorizontal ? x2-x: bounds.x+x),
                     (flipType == FlipVertical   ? y2-y: bounds.y+y),
                     image_getpixel(area, x, y));
}

} }
