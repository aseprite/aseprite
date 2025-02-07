// Aseprite Document Library
// Copyright (c) 2020-2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_UTIL_H_INCLUDED
#define DOC_UTIL_H_INCLUDED
#pragma once

#include "doc/tile.h"

namespace doc {
class Grid;
class Mask;
class Image;
class Tileset;

// Function used to migrate an old tileset format (from internal
// v1.3-alpha3) which doesn't have an empty tile in the zero index
// (notile).
void fix_old_tileset(Tileset* tileset);

// Function used to migrate an old tilemap format (from internal
// v1.3-alpha3) which used a tileset without an empty tile in the
// zero index (notile).
void fix_old_tilemap(Image* image,
                     const Tileset* tileset,
                     const tile_t tileIDMask,
                     const tile_t tileFlagsMask);

// Returns a mask aligned with a given grid, starting from another
// mask not aligned with the grid.
Mask make_aligned_mask(const Grid* grid, const Mask* mask);

} // namespace doc

#endif
