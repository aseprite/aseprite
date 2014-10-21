// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_QUANTIZATION_H_INCLUDED
#define DOC_QUANTIZATION_H_INCLUDED
#pragma once

#include "doc/color_histogram.h"
#include "doc/dithering_method.h"
#include "doc/frame_number.h"
#include "doc/pixel_format.h"

#include <vector>

namespace doc {

  class Image;
  class Palette;
  class RgbMap;
  class Sprite;
  class Stock;

  namespace quantization {

    class PaletteOptimizer {
    public:
      void feedWithImage(Image* image);
      void calculate(Palette* palette, bool has_background_layer);

    private:
      quantization::ColorHistogram<5, 6, 5> m_histogram;
    };

    void create_palette_from_images(
      const std::vector<Image*>& images,
      Palette* palette,
      bool has_background_layer);

    // Creates a new palette suitable to quantize the given RGB sprite to Indexed color.
    Palette* create_palette_from_rgb(
      const Sprite* sprite,
      FrameNumber frameNumber,
      Palette* newPalette);     // Can be NULL to create a new palette

    // Changes the image pixel format. The dithering method is used only
    // when you want to convert from RGB to Indexed.
    Image* convert_pixel_format(
      const Image* src,
      Image* dst,         // Can be NULL to create a new image
      PixelFormat pixelFormat,
      DitheringMethod ditheringMethod,
      const RgbMap* rgbmap,
      const Palette* palette,
      bool is_background);

  } // namespace quantization
} // namespace doc

#endif
