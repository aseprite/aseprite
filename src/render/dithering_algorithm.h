// Aseprite Render Library
// Copyright (c) 2019 Igara Studio S.A.
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_DITHERING_METHOD_H_INCLUDED
#define RENDER_DITHERING_METHOD_H_INCLUDED
#pragma once

namespace render {

  // Dithering algorithms
  enum class DitheringAlgorithm {
    None,
    Ordered,
    Old,
    ErrorDiffusion,
  };

} // namespace render

#endif
