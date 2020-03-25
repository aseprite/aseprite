// Aseprite Document Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_COLOR_SCALES_H_INCLUDED
#define DOC_COLOR_SCALES_H_INCLUDED
#pragma once

#include "base/debug.h"

namespace doc {

  inline int scale_3bits_to_8bits(const int v) {
    ASSERT(v >= 0 && v < 8);
    return (v << 5) | (v << 2) | (v >> 1);
  }

  inline int scale_5bits_to_8bits(const int v) {
    ASSERT(v >= 0 && v < 32);
    return (v << 3) | (v >> 2);
  }

  inline int scale_6bits_to_8bits(const int v) {
    ASSERT(v >= 0 && v < 64);
    return (v << 2) | (v >> 4);
  }

} // namespace doc

#endif
