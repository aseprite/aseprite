// Aseprite Render Library
// Copyright (c) 2019 Igara Studio S.A
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_ERROR_DIFFUSION_H_INCLUDED
#define RENDER_ERROR_DIFFUSION_H_INCLUDED
#pragma once

#include "doc/image_ref.h"
#include "render/ordered_dither.h"

#include <vector>

namespace render {

enum class ErrorDiffusionType : uint8_t {
  FloydSteinberg,
  JarvisJudiceNinke,
  Stucki,
  Atkinson,
  Burkes,
  Sierra
};

// Error diffusion matrix structure
struct ErrorDiffusionMatrix {
  int width, height;
  int centerX, centerY;
  std::vector<std::vector<int>> coefficients;
  int divisor;

  ErrorDiffusionMatrix(int w,
                       int h,
                       int cx,
                       int cy,
                       const std::vector<std::vector<int>>& coeff,
                       int div)
    : width(w)
    , height(h)
    , centerX(cx)
    , centerY(cy)
    , coefficients(coeff)
    , divisor(div)
  {
  }
};

class ErrorDiffusionDither : public DitheringAlgorithmBase {
public:
  ErrorDiffusionDither(ErrorDiffusionType type, int transparentIndex);
  int dimensions() const override { return 2; }
  void start(const doc::Image* srcImage, doc::Image* dstImage, const double factor) override;
  void finish() override;
  doc::color_t ditherRgbToIndex2D(const int x,
                                  const int y,
                                  const doc::RgbMap* rgbmap,
                                  const doc::Palette* palette,
                                  const int direction) override;

private:
  const ErrorDiffusionMatrix& getCurrentMatrix() const;

  int m_transparentIndex;
  ErrorDiffusionType m_diffusionType;
  const doc::Image* m_srcImage;
  int m_width, m_lastY;
  int m_currentRowOffset;
  static const int kChannels = 4;
  std::vector<int> m_err[kChannels];
  int m_factor;
};

} // namespace render

#endif
