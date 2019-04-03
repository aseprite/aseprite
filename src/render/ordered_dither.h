// Aseprite Render Library
// Copyright (c) 2019 Igara Studio S.A.
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_ORDERED_DITHER_H_INCLUDED
#define RENDER_ORDERED_DITHER_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "render/task_delegate.h"

namespace render {

  class Dithering;
  class DitheringMatrix;

  class DitheringAlgorithmBase {
  public:
    virtual ~DitheringAlgorithmBase() { }

    virtual int dimensions() const { return 1; }
    virtual bool zigZag() const { return false; }

    virtual void start(
      const doc::Image* srcImage,
      doc::Image* dstImage,
      const double factor) { }

    virtual void finish() { }

    virtual doc::color_t ditherRgbPixelToIndex(
      const DitheringMatrix& matrix,
      const doc::color_t color,
      const int x, const int y,
      const doc::RgbMap* rgbmap,
      const doc::Palette* palette) { return 0; }

    virtual doc::color_t ditherRgbToIndex2D(
      const int x, const int y,
      const doc::RgbMap* rgbmap,
      const doc::Palette* palette) { return 0; }
  };

  class OrderedDither : public DitheringAlgorithmBase {
  public:
    OrderedDither(int transparentIndex = -1);
    doc::color_t ditherRgbPixelToIndex(
      const DitheringMatrix& matrix,
      const doc::color_t color,
      const int x,
      const int y,
      const doc::RgbMap* rgbmap,
      const doc::Palette* palette) override;
  private:
    int m_transparentIndex;
  };

  class OrderedDither2 : public DitheringAlgorithmBase {
  public:
    OrderedDither2(int transparentIndex = -1);
    doc::color_t ditherRgbPixelToIndex(
      const DitheringMatrix& matrix,
      const doc::color_t color,
      const int x,
      const int y,
      const doc::RgbMap* rgbmap,
      const doc::Palette* palette) override;
  private:
    int m_transparentIndex;
  };

  void dither_rgb_image_to_indexed(
    DitheringAlgorithmBase& algorithm,
    const Dithering& dithering,
    const doc::Image* srcImage,
    doc::Image* dstImage,
    const doc::RgbMap* rgbmap,
    const doc::Palette* palette,
    TaskDelegate* delegate = nullptr);

} // namespace render

#endif
