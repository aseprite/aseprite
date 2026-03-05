// Aseprite Document Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "doc/brush_pattern.h"

namespace doc {

Pattern::Pattern(uint8_t* bits, int width, int height)
{
  m_image.reset(Image::create(PixelFormat::IMAGE_BITMAP, width, height));
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      m_image->putPixel(j, i, bits[i * width + j]);
    }
  }
}

} // namespace doc
