// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/tile_primitives.h"

#include "doc/cel.h"
#include "doc/color.h"
#include "doc/grid.h"
#include "doc/image.h"
#include "doc/layer_tilemap.h"

#define TILE_TRACE(...) // TRACE(__VA_ARGS__)

namespace doc {

bool get_tile_pixel(
  // Input
  const Image* tilemapImage,
  const Tileset* tileset,
  const Grid& grid,
  const gfx::PointF& canvasPos,
  // Output
  tile_index& ti,
  tile_index& tf,
  color_t& tileImageColor)
{
  const gfx::Point tilePos = grid.canvasToTile(gfx::Point(canvasPos));
  TILE_TRACE("TILE: tilePos=(%d %d)\n", tilePos.x, tilePos.y);
  if (!tilemapImage->bounds().contains(tilePos))
    return false;

  const doc::tile_t t = doc::get_pixel(tilemapImage, tilePos.x, tilePos.y);
  ti = doc::tile_geti(t);
  tf = doc::tile_getf(t);

  TILE_TRACE("TILE: tile=0x%08x index=%d flags=0x%08x\n", t, ti, tf);

  const doc::ImageRef tile = tileset->get(ti);
  if (!tile)
    return false;

  const gfx::Point tileStart = grid.tileToCanvas(tilePos);
  gfx::Point ipos = gfx::Point(canvasPos) - tileStart;
  if (tf & doc::tile_f_xflip) {
    ipos.x = tile->width() - ipos.x - 1;
  }
  if (tf & doc::tile_f_yflip) {
    ipos.y = tile->height() - ipos.y - 1;
  }
  if (tf & doc::tile_f_dflip) {
    std::swap(ipos.x, ipos.y);
  }

  tileImageColor = get_pixel(tile.get(), ipos.x, ipos.y);

  TILE_TRACE("TILE: tileImagePos=%d %d\n", ipos.x, ipos.y);
  TILE_TRACE("TILE: tileImageColor=%d\n", tileImageColor);
  return true;
}

bool get_tile_pixel(
  // Input
  const Cel* cel,
  const gfx::PointF& canvasPos,
  // Output
  tile_index& ti,
  tile_index& tf,
  color_t& tileImageColor)
{
  if (!cel || !cel->layer()->isTilemap() || !cel->image()->isTilemap())
    return false;

  Tileset* tileset = static_cast<LayerTilemap*>(cel->layer())->tileset();
  if (!tileset)
    return false;

  return get_tile_pixel(cel->image(), tileset, cel->grid(), canvasPos, ti, tf, tileImageColor);
}

} // namespace doc
