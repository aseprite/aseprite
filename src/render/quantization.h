// Aseprite Rener Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_QUANTIZATION_H_INCLUDED
#define RENDER_QUANTIZATION_H_INCLUDED
#pragma once

#include "doc/dithering_method.h"
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
  using namespace doc;

  class PaletteOptimizerDelegate {
  public:
    virtual ~PaletteOptimizerDelegate() { }
    virtual void onPaletteOptimizerProgress(double progress) = 0;
    virtual bool onPaletteOptimizerContinue() = 0;
  };

  class PaletteOptimizer {
  public:
    void feedWithImage(Image* image, bool withAlpha);
    void feedWithRgbaColor(color_t color);
    void calculate(Palette* palette, int maskIndex, PaletteOptimizerDelegate* delegate);

  private:
    ColorHistogram<5, 6, 5, 5> m_histogram;
  };

  // Creates a new palette suitable to quantize the given RGB sprite to Indexed color.
  Palette* create_palette_from_sprite(
    const Sprite* sprite,
    frame_t fromFrame,
    frame_t toFrame,
    bool withAlpha,
    Palette* newPalette, // Can be NULL to create a new palette
    PaletteOptimizerDelegate* delegate);

  // Changes the image pixel format. The dithering method is used only
  // when you want to convert from RGB to Indexed.
  Image* convert_pixel_format(
    const Image* src,
    Image* dst,         // Can be NULL to create a new image
    PixelFormat pixelFormat,
    DitheringMethod ditheringMethod,
    const RgbMap* rgbmap,
    const Palette* palette,
    bool is_background,
    color_t new_mask_color);

} // namespace render

#endif
