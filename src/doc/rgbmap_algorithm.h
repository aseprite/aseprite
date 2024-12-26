// Aseprite Document Library
// Copyright (c) 2020-2021 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_RGBMAP_ALGORITHM_H_INCLUDED
#define DOC_RGBMAP_ALGORITHM_H_INCLUDED
#pragma once

namespace doc {

enum class RgbMapAlgorithm {
  DEFAULT = 0,
  RGB5A3 = 1,
  OCTREE = 2,
};

} // namespace doc

#endif
