// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BRUSH_PATTERN_H_INCLUDED
#define DOC_BRUSH_PATTERN_H_INCLUDED
#pragma once

#include "doc/image_ref.h"

#include <memory>

namespace doc {

class Pattern;
using PatternRef = std::shared_ptr<Pattern>;

enum class BrushPattern {
  ALIGNED_TO_SRC = 0,
  ALIGNED_TO_DST = 1,
  PAINT_BRUSH = 2,

  DEFAULT = ALIGNED_TO_SRC, // Default for preferences
  DEFAULT_FOR_UI = ALIGNED_TO_SRC,
  DEFAULT_FOR_SCRIPTS = PAINT_BRUSH,
};

class Pattern {
public:
  Pattern(uint8_t* bits, int width, int height);

  doc::ImageRef image() const { return m_image; }

private:
  doc::ImageRef m_image = nullptr;
};

} // namespace doc

#endif
