// Aseprite Document Library
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_MODIFY_SELECTION_H_INCLUDED
#define DOC_ALGORITHM_MODIFY_SELECTION_H_INCLUDED
#pragma once

#include "doc/brush_type.h"
#include "doc/color.h"
#include "gfx/point.h"

namespace doc {
  class Mask;
  namespace algorithm {

    enum class SelectionModifier {
      Border,
      Expand,
      Contract,
    };

    void modify_selection(const SelectionModifier modifier,
                          const Mask* srcMask,
                          Mask* dstMask,
                          const int radius,
                          const BrushType brush);

  } // namespace algorithm
} // namespace doc

#endif
