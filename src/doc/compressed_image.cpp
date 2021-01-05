// Aseprite Document Library
// Copyright (c) 2021 Igara Studio S.A.
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

CompressedImage::CompressedImage(const Image* image,
                                 const Image* maskBitmap,
                                 bool diffColors,
                                 Symmetry symmetry)
  : m_image(image)
{
  color_t c1, c2, mask = image->maskColor();

  int initialXIndex, initialYIndex, finalXIndex, finalYIndex, incrementX, incrementY;

  switch (symmetry) {
    case Symmetry::NONE: {
      initialXIndex = 0;
      initialYIndex = 0;
      finalXIndex = image->width()-1;
      finalYIndex = image->height()-1;
      incrementX = 1;
      incrementY = 1;
      break;
    }
    case Symmetry::X_SYMMETRY: {
      initialXIndex = image->width()-1;
      initialYIndex = 0;
      finalXIndex = 0;
      finalYIndex = image->height()-1;
      incrementX = -1;
      incrementY = 1;
      break;
    }
    case Symmetry::Y_SYMMETRY: {
      initialXIndex = 0;
      initialYIndex = image->height()-1;
      finalXIndex = image->width()-1;
      finalYIndex = 0;
      incrementX = 1;
      incrementY = -1;
      break;
    }
    case Symmetry::XY_SYMMETRY: {
      initialXIndex = image->width()-1;
      initialYIndex = image->height()-1;
      finalXIndex = 0;
      finalYIndex = 0;
      incrementX = -1;
      incrementY = -1;
      break;
    }
  }

  for (int y = initialYIndex; y * incrementY <= finalYIndex; y += incrementY) {
    Scanline scanline(initialYIndex + y * incrementY);

    for (int x = initialXIndex; x * incrementX <= finalXIndex; ) {
      if (maskBitmap && !get_pixel_fast<BitmapTraits>(maskBitmap, x, y)) {
        x += incrementX;
        continue;
      }

      c1 = get_pixel(image, x, y);

      if (!maskBitmap && c1 == mask) {
        x += incrementX;
        continue;
      }

      scanline.color = c1;
      scanline.x = initialXIndex + x * incrementX;

      for (x += incrementX; x * incrementX <= finalXIndex; x += incrementX) {
        if (maskBitmap && !get_pixel_fast<BitmapTraits>(maskBitmap, x, y))
          break;

        c2 = get_pixel(image, x, y);

        if (diffColors && c1 != c2)
          break;

        if (!diffColors && !maskBitmap && c2 == mask)
          break;
      }

      scanline.w = initialXIndex + x * incrementX - scanline.x;
      if (scanline.w > 0)
        m_scanlines.push_back(scanline);
    }
  }
}

} // namespace doc
