// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TILESET_MODE_H_INCLUDED
#define APP_TILESET_MODE_H_INCLUDED
#pragma once

namespace app {

// These modes are available edition modes for the tileset when an
// tilemap is edited.
enum class TilesetMode {
  Manual, // Modify existent tiles (don't create new ones)
  Auto,   // Add/remove tiles automatically when needed
  Stack,  // Stack modified tiles as new ones
};

} // namespace app

#endif
