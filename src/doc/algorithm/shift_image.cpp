// Aseprite Document Library
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algorithm/shift_image.h"

#include "gfx/rect.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/primitives.h"

#include <vector>

namespace doc {
namespace algorithm {

void shift_image_with_mask(Image* image, const Mask* mask, int dx, int dy)
{
  gfx::Rect bounds = mask->bounds();
  if (bounds.isEmpty())
    return;

  // To simplify the algorithm we use a copy of the original image, we
  // could avoid this copy swapping rows and columns.
  ImageRef crop(crop_image(image, bounds.x, bounds.y, bounds.w, bounds.h,
                           image->maskColor()));

  int u = dx;
  int v = dy;
  while (u < 0) u += bounds.w;
  while (v < 0) v += bounds.h;

  for (int y=0; y<bounds.h; ++y) {
    for (int x=0; x<bounds.w; ++x) {
      put_pixel(image,
                bounds.x + ((u + x) % bounds.w),
                bounds.y + ((v + y) % bounds.h),
                get_pixel(crop.get(), x, y));
    }
  }
}

} // namespace algorithm
} // namespace doc
