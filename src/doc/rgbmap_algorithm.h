// Aseprite Document Library
// Copyright (c) 2020 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_RGBMAP_ALGORITHM_H_INCLUDED
#define DOC_RGBMAP_ALGORITHM_H_INCLUDED
#pragma once

namespace doc {

  enum class RgbMapAlgorithm {
    RGB5A3,
    OCTREE,
    DEFAULT = RGB5A3
  };

} // namespace doc

#endif
