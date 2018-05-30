// Aseprite Document Library
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algorithm/modify_selection.h"

#include "doc/image_impl.h"
#include "doc/mask.h"
#include "doc/primitives.h"

#include <memory>

namespace doc {
namespace algorithm {

// TODO create morphological operators/functions in "doc" namespace
// TODO the impl is not optimal, but is good enough as a first version
void modify_selection(const SelectionModifier modifier,
                      const Mask* srcMask,
                      Mask* dstMask,
                      const int radius,
                      const doc::BrushType brush)
{
  const doc::Image* srcImage = srcMask->bitmap();
  doc::Image* dstImage = dstMask->bitmap();
  const gfx::Point offset =
    srcMask->bounds().origin() -
    dstMask->bounds().origin();

  // Image bounds to clip get/put pixels
  const gfx::Rect srcBounds = srcImage->bounds();

  // Create a kernel
  const int size = 2*radius+1;
  std::unique_ptr<doc::Image> kernel(doc::Image::create(IMAGE_BITMAP, size, size));
  doc::clear_image(kernel.get(), 0);
  if (brush == doc::kCircleBrushType)
    doc::fill_ellipse(kernel.get(), 0, 0, size-1, size-1, 1);
  else
    doc::fill_rect(kernel.get(), 0, 0, size-1, size-1, 1);
  doc::put_pixel(kernel.get(), radius, radius, 0);

  int total = 0;                // Number of 1s in the kernel image
  for (int v=0; v<size; ++v)
    for (int u=0; u<size; ++u)
      total += kernel->getPixel(u, v);

  for (int y=-radius; y<srcBounds.h+radius; ++y) {
    for (int x=-radius; x<srcBounds.w+radius; ++x) {
      doc::color_t c;
      if (srcBounds.contains(x, y))
        c = srcImage->getPixel(x, y);
      else
        c = 0;

      int accum = 0;
      for (int v=0; v<size; ++v) {
        for (int u=0; u<size; ++u) {
          if (kernel->getPixel(u, v)) {
            if (srcBounds.contains(x+u-radius, y+v-radius))
              accum += srcImage->getPixel(x-radius+u, y-radius+v);
          }
        }
      }

      switch (modifier) {
        case SelectionModifier::Border: {
          c = (c && accum < total) ? 1: 0;
          break;
        }
        case SelectionModifier::Expand: {
          c = (c || accum > 0) ? 1: 0;
          break;
        }
        case SelectionModifier::Contract: {
          c = (c && accum == total) ? 1: 0;
          break;
        }
      }

      if (c)
        doc::put_pixel(dstImage,
                       offset.x+x,
                       offset.y+y, 1);
    }
  }
}

} // namespace algorithm
} // namespace app
