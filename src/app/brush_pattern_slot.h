// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_BRUSH_PATTERN_SLOT_H_INCLUDED
#define APP_BRUSH_PATTERN_SLOT_H_INCLUDED
#pragma once

#include "doc/brush_pattern.h"
#include "doc/image_ref.h"

namespace app {

// Brush pattern slot
class BrushPatternSlot {
public:
  BrushPatternSlot() {}
  BrushPatternSlot(const doc::PatternRef& pattern);

  doc::PatternRef pattern() const { return m_pattern; }

  bool hasPattern() { return m_pattern != nullptr; }

private:
  doc::PatternRef m_pattern = nullptr;
};

} // namespace app

#endif
