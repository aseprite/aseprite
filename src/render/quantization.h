// Aseprite Rener Library
// Copyright (c) 2001-2015, 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_QUANTIZATION_H_INCLUDED
#define RENDER_QUANTIZATION_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/pixel_format.h"
#include "render/color_histogram.h"
#include "render/dithering_algorithm.h"

#include <vector>

namespace doc {
  class Image;
  class Palette;
  class RgbMap;
  class Sprite;
}

namespace render {

  class PaletteOptimizerDelegate {
  public:
    virtual ~PaletteOptimizerDelegate() { }
    virtual void onPaletteOptimizerProgress(double progress) = 0;
    virtual bool onPaletteOptimizerContinue() = 0;
  };

  class PaletteOptimizer {
  public:
    void feedWithImage(doc::Image* image, bool withAlpha);
    void feedWithRgbaColor(doc::color_t color);
    void calculate(doc::Palette* palette,
                   int maskIndex,
                   render::PaletteOptimizerDelegate* delegate);

  private:
    render::ColorHistogram<5, 6, 5, 5> m_histogram;
  };

  // Creates a new palette suitable to quantize the given RGB sprite to Indexed color.
  doc::Palette* create_palette_from_sprite(
    const doc::Sprite* sprite,
    const doc::frame_t fromFrame,
    const doc::frame_t toFrame,
    const bool withAlpha,
    doc::Palette* newPalette, // Can be NULL to create a new palette
    render::PaletteOptimizerDelegate* delegate);

  // Changes the image pixel format. The dithering method is used only
  // when you want to convert from RGB to Indexed.
  Image* convert_pixel_format(
    const doc::Image* src,
    doc::Image* dst,         // Can be NULL to create a new image
    doc::PixelFormat pixelFormat,
    render::DitheringAlgorithm ditheringAlgorithm,
    const doc::RgbMap* rgbmap,
    const doc::Palette* palette,
    bool is_background,
    doc::color_t new_mask_color,
    bool* stopFlag = nullptr);

} // namespace render

#endif
