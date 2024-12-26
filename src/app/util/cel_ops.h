// Aseprite
// Copyright (C) 2019-2021  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_CEL_OPS_H_INCLUDED
#define APP_UTIL_CEL_OPS_H_INCLUDED
#pragma once

#include "app/tilemap_mode.h"
#include "app/tileset_mode.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "gfx/point.h"
#include "gfx/region.h"

#include <functional>
#include <vector>

namespace doc {
class Cel;
class Layer;
class LayerTilemap;
class PalettePicks;
class Sprite;
class Tileset;
} // namespace doc

namespace app {
class CmdSequence;

typedef std::function<doc::ImageRef(const doc::ImageRef& origTile,
                                    const gfx::Rect& tileBoundsInCanvas)>
  GetTileImageFunc;

void create_region_with_differences(const doc::Image* a,
                                    const doc::Image* b,
                                    const gfx::Rect& bounds,
                                    gfx::Region& output);

// Creates a new image of the given cel
doc::ImageRef crop_cel_image(const doc::Cel* cel, const doc::color_t bgcolor);

// The "cmds" is used in case that new tiles must be added in the
// dstLayer tilesets.
doc::Cel* create_cel_copy(CmdSequence* cmds,
                          const doc::Cel* srcCel,
                          const doc::Sprite* dstSprite,
                          doc::Layer* dstLayer,
                          const doc::frame_t dstFrame);

// Draws an image creating new tiles.
void draw_image_into_new_tilemap_cel(CmdSequence* cmds,
                                     doc::LayerTilemap* dstLayer,
                                     doc::Cel* dstCel,
                                     const doc::Image* srcImage,
                                     const gfx::Point& gridOrigin,
                                     const gfx::Point& srcImagePos,
                                     const gfx::Rect& canvasBounds,
                                     doc::ImageRef& newTilemap);

void modify_tilemap_cel_region(CmdSequence* cmds,
                               doc::Cel* cel,
                               doc::Tileset* previewTileset, // Temporary tileset that can be used
                                                             // for preview
                               const gfx::Region& region,
                               const TilesetMode tilesetMode,
                               const GetTileImageFunc& getTileImage,
                               const gfx::Region& forceRegion = gfx::Region());

void clear_mask_from_cel(CmdSequence* cmds,
                         doc::Cel* cel,
                         const TilemapMode tilemapMode,
                         const TilesetMode tilesetMode);

void move_tiles_in_tileset(CmdSequence* cmds,
                           doc::Tileset* tileset,
                           doc::PalettePicks& picks,
                           int& currentEntry,
                           int beforeIndex);

void copy_tiles_in_tileset(CmdSequence* cmds,
                           doc::Tileset* tileset,
                           doc::PalettePicks& picks,
                           int& currentEntry,
                           int beforeIndex);

} // namespace app

#endif
