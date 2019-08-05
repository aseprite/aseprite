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
    Manual,   // Modify existent tiles (don't create new ones)
    Semi,     // Auto-generate new tiles if needed, edit existent ones
    Auto,     // Auto-generate new and modified tiles
  };

} // namespace app

#endif
