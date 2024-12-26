// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "tileset_utils.h"

#include <fmt/format.h>
#include <string>

namespace app {

std::string tileset_label(const doc::Tileset* tileset, doc::tileset_index tsi)
{
  std::string tilemapsNames;
  for (auto layer : tileset->sprite()->allTilemaps()) {
    auto tilemap = static_cast<doc::LayerTilemap*>(layer);
    if (tilemap->tilesetIndex() == tsi) {
      tilemapsNames += tilemap->name() + ", ";
    }
  }
  if (!tilemapsNames.empty()) {
    tilemapsNames = tilemapsNames.substr(0, tilemapsNames.size() - 2);
  }

  std::string name = tileset->name();
  if (!tilemapsNames.empty()) {
    name += " (" + tilemapsNames + ")";
  }

  return fmt::format("#{0} ({1}x{2}): {3}",
                     tsi,
                     tileset->grid().tileSize().w,
                     tileset->grid().tileSize().h,
                     name);
}

} // namespace app
