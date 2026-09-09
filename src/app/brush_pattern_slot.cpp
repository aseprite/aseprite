// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/brush_pattern_slot.h"

#include <memory>

namespace app {

using namespace doc;

BrushPatternSlot::BrushPatternSlot(const PatternRef& pattern) : m_pattern(pattern)
{
}

} // namespace app
