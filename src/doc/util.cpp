// Aseprite Document Library
// Copyright (c) 2020 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "doc/util.h"

#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/tileset.h"

namespace doc {

void fix_old_tileset(
  Tileset* tileset)
{
  // Check if the first tile is already the empty tile, in this
  // case we can use this tileset as a new tileset without any
  // conversion.
  if (tileset->size() > 0 && is_empty_image(tileset->get(0).get())) {
    tileset->setBaseIndex(1);
  }
  else {
    // Add the empty tile in the index = 0
    tileset->insert(0, tileset->makeEmptyTile());

    // The tile 1 will be displayed as tile 0 in the editor
    tileset->setBaseIndex(0);
  }
}

void fix_old_tilemap(
  Image* image,
  const Tileset* tileset,
  const tile_t tileIDMask,
  const tile_t tileFlagsMask)
{
  int delta = (tileset->baseIndex() == 0 ? 1: 0);

  // Convert old empty tile (0xffffffff) to new empty tile (index 0 = notile)
  transform_image<TilemapTraits>(
    image,
    [tileIDMask, tileFlagsMask, delta](color_t c) -> color_t {
      color_t res = c;
      if (c == 0xffffffff)
        res = notile;
      else
        res = (c & tileFlagsMask) | ((c & tileIDMask)+delta);
      return res;
    });
}

} // namespace dooc
