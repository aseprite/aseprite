// Aseprite Render Library
// Copyright (c) 2019-2020 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_GRADIENT_H_INCLUDED
#define RENDER_GRADIENT_H_INCLUDED
#pragma once

#include "doc/brush.h"
#include "doc/color.h"
#include "gfx/point.h"

namespace doc {
  class Image;
}

namespace render {

class DitheringMatrix;

enum class GradientType {
  Linear,
  Radial,
};

void render_rgba_gradient(
  doc::Image* img,
  const gfx::Point imgPos,
  const gfx::Point p0,
  const gfx::Point p1,
  doc::color_t c0,
  doc::color_t c1,
  const render::DitheringMatrix& matrix,
  const GradientType type);

void render_rgba_linear_gradient(
  doc::Image* img,
  const gfx::Point imgPos,
  const gfx::Point p0,
  const gfx::Point p1,
  doc::color_t c0,
  doc::color_t c1,
  const render::DitheringMatrix& matrix);

void render_rgba_radial_gradient(
  doc::Image* img,
  const gfx::Point imgPos,
  const gfx::Point p0,
  const gfx::Point p1,
  doc::color_t c0,
  doc::color_t c1,
  const render::DitheringMatrix& matrix);

void convert_bitmap_brush_to_dithering_brush(
  doc::Brush* brush,
  const doc::PixelFormat pixelFormat,
  const render::DitheringMatrix& matrix,
  const float f,
  const doc::color_t c0,
  const doc::color_t c1);

} // namespace render

#endif
