// Aseprite Document Library
// Copyright (c) 2019-2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TILE_H_INCLUDED
#define DOC_TILE_H_INCLUDED
#pragma once

#include "base/ints.h"

namespace doc {

typedef uint32_t tile_t; // Same as color_t
typedef uint32_t tile_index;
typedef uint32_t tileset_index;
typedef uint32_t tile_flags;

const uint32_t tile_i_shift = 0;  // Tile index
const uint32_t tile_f_shift = 28; // Tile flags (flips)

const uint32_t notile = 0;
const uint32_t tile_i_mask = 0x1fffffff;
const uint32_t tile_f_mask = 0xe0000000; // 3 flags
const uint32_t tile_f_xflip = 0x80000000;
const uint32_t tile_f_yflip = 0x40000000;
const uint32_t tile_f_dflip = 0x20000000;

inline tile_index tile_geti(const tile_t t)
{
  return ((t & tile_i_mask) >> tile_i_shift);
}

inline tile_flags tile_getf(const tile_t t)
{
  return (t & tile_f_mask);
}

inline tile_t tile(const tile_index i, const tile_flags f)
{
  return (((i << tile_i_shift) & tile_i_mask) | (f & tile_f_mask));
}

} // namespace doc

#endif
