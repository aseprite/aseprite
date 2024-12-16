// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/compressed_image.h"

#include "doc/primitives.h"
#include "doc/primitives_fast.h"

namespace doc {

CompressedImage::CompressedImage(const Image* image, const Image* maskBitmap, bool diffColors)
  : m_image(image)
{
  color_t c1, c2, mask = image->maskColor();

  for (int y = 0; y < image->height(); ++y) {
    Scanline scanline(y);

    for (int x = 0; x < image->width();) {
      if (maskBitmap && !get_pixel_fast<BitmapTraits>(maskBitmap, x, y)) {
        ++x;
        continue;
      }

      c1 = get_pixel(image, x, y);

      if (!maskBitmap && c1 == mask) {
        ++x;
        continue;
      }

      scanline.color = c1;
      scanline.x = x;

      for (++x; x < image->width(); ++x) {
        if (maskBitmap && !get_pixel_fast<BitmapTraits>(maskBitmap, x, y))
          break;

        c2 = get_pixel(image, x, y);

        if (diffColors && c1 != c2)
          break;

        if (!diffColors && !maskBitmap && c2 == mask)
          break;
      }

      scanline.w = x - scanline.x;
      if (scanline.w > 0)
        m_scanlines.push_back(scanline);
    }
  }
}

} // namespace doc
