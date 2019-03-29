// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TILES_MODE_H_INCLUDED
#define APP_TILES_MODE_H_INCLUDED
#pragma once

namespace app {

  // These modes are available edition modes for the tileset when an
  // tilemap is edited.
  enum class TilesetMode {
    Locked,           // Cannot edit the tileset (don't modify or add new tiles)
    ModifyExistent,   // Modify existent tiles (don't create new ones)
    //GenerateNewTiles, // Auto-generate new tiles, edit existent ones
    GenerateAllTiles, // Auto-generate new and modified tiles
  };

} // namespace app

#endif
