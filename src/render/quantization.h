// Aseprite Rener Library
// Copyright (c) 2019-2020  Igara Studio S.A.
// Copyright (c) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_QUANTIZATION_H_INCLUDED
#define RENDER_QUANTIZATION_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/pixel_format.h"
#include "render/color_histogram.h"

#include <vector>

namespace doc {
  class Image;
  class Palette;
  class RgbMap;
  class Sprite;
}

namespace render {
  class Dithering;
  class TaskDelegate;

  class PaletteOptimizer {
  public:
    void feedWithImage(doc::Image* image, bool withAlpha);
    void feedWithRgbaColor(doc::color_t color);
    void calculate(doc::Palette* palette, int maskIndex);

  private:
    render::ColorHistogram<5, 6, 5, 5> m_histogram;
    bool m_withAlpha = false;
  };

  // Creates a new palette suitable to quantize the given RGB sprite to Indexed color.
  doc::Palette* create_palette_from_sprite(
    const doc::Sprite* sprite,
    const doc::frame_t fromFrame,
    const doc::frame_t toFrame,
    const bool withAlpha,
    doc::Palette* newPalette, // Can be NULL to create a new palette
    TaskDelegate* delegate,
    const bool newBlend);

  // Changes the image pixel format. The dithering method is used only
  // when you want to convert from RGB to Indexed.
  Image* convert_pixel_format(
    const doc::Image* src,
    doc::Image* dst,         // Can be NULL to create a new image
    doc::PixelFormat pixelFormat,
    const render::Dithering& dithering,
    const doc::RgbMap* rgbmap,
    const doc::Palette* palette,
    bool is_background,
    doc::color_t new_mask_color,
    doc::rgba_to_graya_func toGray = nullptr,
    TaskDelegate* delegate = nullptr);

} // namespace render

#endif
