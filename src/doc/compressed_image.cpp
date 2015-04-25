// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/compressed_image.h"

#include "doc/primitives.h"

namespace doc {

CompressedImage::CompressedImage(const Image* image)
  : m_image(image)
{
  color_t mask = image->maskColor();

  for (int y=0; y<image->height(); ++y) {
    Scanline scanline(y);

    for (int x=0; x<image->width(); ++x) {
      scanline.color = get_pixel(image, x, y);

      if (scanline.color != mask) {
        scanline.x = x;

        for (; x<image->width(); ++x)
          if (!get_pixel(image, x, y))
            break;

        scanline.w = x - scanline.x;
        m_scanlines.push_back(scanline);
      }
    }
  }
}

} // namespace doc
