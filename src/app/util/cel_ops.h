// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_CEL_OPS_H_INCLUDED
#define APP_UTIL_CEL_OPS_H_INCLUDED
#pragma once

#include "app/tileset_mode.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "gfx/point.h"
#include "gfx/region.h"

#include <functional>

namespace doc {
  class Cel;
  class Layer;
  class LayerTilemap;
  class Sprite;
}

namespace app {
  class CmdSequence;

  typedef std::function<doc::ImageRef(const doc::ImageRef& origTile,
                                      const gfx::Rect& tileBoundsInCanvas)> GetTileImageFunc;

  // Creates a new image of the given cel
  doc::ImageRef crop_cel_image(
    const doc::Cel* cel,
    const doc::color_t bgcolor);

  // The "cmds" is used in case that new tiles must be added in the
  // dstLayer tilesets.
  doc::Cel* create_cel_copy(
    CmdSequence* cmds,
    const doc::Cel* srcCel,
    const doc::Sprite* dstSprite,
    doc::Layer* dstLayer,
    const doc::frame_t dstFrame);

  // Draws an image creating new tiles.
  void draw_image_into_new_tilemap_cel(
    CmdSequence* cmds,
    doc::LayerTilemap* dstLayer,
    doc::Cel* dstCel,
    const doc::Image* srcImage,
    const gfx::Point& originOffset,
    const gfx::Point& srcImagePos,
    const gfx::Rect& canvasBounds,
    doc::ImageRef& newTilemap);

  void modify_tilemap_cel_region(
    CmdSequence* cmds,
    doc::Cel* cel,
    const gfx::Region& region,
    const TilesetMode tilesetMode,
    const GetTileImageFunc& getTileImage);

  void clear_mask_from_cel(
    CmdSequence* cmds,
    doc::Cel* cel,
    const TilesetMode tilesetMode);

} // namespace app

#endif
