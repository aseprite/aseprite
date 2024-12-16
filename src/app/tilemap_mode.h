// Aseprite
// Copyright (c) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TILEMAP_MODE_H_INCLUDED
#define APP_TILEMAP_MODE_H_INCLUDED
#pragma once

namespace app {

// Should we edit the pixels or the tiles of the tilemap?
enum class TilemapMode {
  Pixels, // Edit tile pixels
  Tiles,  // Edit tiles
};

} // namespace app

#endif
