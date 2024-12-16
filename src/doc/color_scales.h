// Aseprite Document Library
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_COLOR_SCALES_H_INCLUDED
#define DOC_COLOR_SCALES_H_INCLUDED
#pragma once

#include "base/debug.h"

namespace doc {

constexpr inline int scale_3bits_to_8bits(const int v)
{
  ASSERT(v >= 0 && v < 8);
  return (v << 5) | (v << 2) | (v >> 1);
}

constexpr inline int scale_5bits_to_8bits(const int v)
{
  ASSERT(v >= 0 && v < 32);
  return (v << 3) | (v >> 2);
}

constexpr inline int scale_6bits_to_8bits(const int v)
{
  ASSERT(v >= 0 && v < 64);
  return (v << 2) | (v >> 4);
}

template<typename T>
constexpr inline int scale_xbits_to_8bits(const int x, const T v)
{
  switch (x) {
    case 3: return scale_3bits_to_8bits(v);
    case 5: return scale_5bits_to_8bits(v);
    case 6: return scale_6bits_to_8bits(v);
    default:
      if (x < 8) {
        return (255.0 * double(v) / double((T(1) << x) - 1));
      }
      else {
        // To reduce precision (e.g. from 10 bits to 8 bits) we just
        // remove least significant bits.
        return v >> (x - 8);
      }
  }
}

} // namespace doc

#endif
