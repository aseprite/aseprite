// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TILE_PRIMITIVES_H_INCLUDED
#define DOC_TILE_PRIMITIVES_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/tile.h"
#include "gfx/point.h"

namespace doc {

class Cel;
class Grid;
class Image;
class Tileset;

// Returns true if "canvasPos" is inside the given tilemap with the
// given grid settings, and the tile in that "canvasPos" is.
//
// Input:
// * tilemapImage: a tilemap layer
// * tileset: a tileset for the tilemap to get tiles
// * grid: the grid settings (e.g. cel->grid())
// * canvasPos: a position on the sprite
//
// Output:
// * ti: the tile index in the "canvasPos"
// * tf: the flags/flips of that tile
// * tileImageColor: the pixel color above the tile image
//   on "canvasPos" after applying the tile flips (tf)
bool get_tile_pixel(
  // Input
  const Image* tilemapImage,
  const Tileset* tileset,
  const Grid& grid,
  const gfx::PointF& canvasPos,
  // Output
  tile_index& ti,
  tile_index& tf,
  color_t& tileImageColor);

bool get_tile_pixel(
  // Input
  const Cel* cel,
  const gfx::PointF& canvasPos,
  // Output
  tile_index& ti,
  tile_index& tf,
  color_t& tileImageColor);

} // namespace doc

#endif
