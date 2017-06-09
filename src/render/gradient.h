// Aseprite Render Library
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_GRADIENT_H_INCLUDED
#define RENDER_GRADIENT_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "gfx/point.h"

namespace doc {
  class Image;
}

namespace render {

class DitheringMatrix;

void render_rgba_linear_gradient(
  doc::Image* img,
  const gfx::Point p0,
  const gfx::Point p1,
  doc::color_t c0,
  doc::color_t c1,
  const render::DitheringMatrix& matrix);

} // namespace render

#endif
