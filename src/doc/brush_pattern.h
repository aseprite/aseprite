// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BRUSH_PATTERN_H_INCLUDED
#define DOC_BRUSH_PATTERN_H_INCLUDED
#pragma once

namespace doc {

  enum class BrushPattern {
    DEFAULT = 0,
    ALIGNED_TO_SRC = 0,
    ALIGNED_TO_DST = 1,
    PAINT_BRUSH = 2,
  };

} // namespace doc

#endif
