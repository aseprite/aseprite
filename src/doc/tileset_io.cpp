// Aseprite Document Library
// Copyright (C) 2019-2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/tileset_io.h"

#include "base/serialization.h"
#include "doc/cancel_io.h"
#include "doc/grid_io.h"
#include "doc/image_io.h"
#include "doc/string_io.h"
#include "doc/subobjects_io.h"
#include "doc/tileset.h"
#include "doc/user_data_io.h"
#include "doc/util.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

bool write_tileset(std::ostream& os, const Tileset* tileset, CancelIO* cancel)
{
  write32(os, tileset->id());
  write32(os, tileset->size());
  write_grid(os, tileset->grid());

  for (tile_index ti = 0; ti < tileset->size(); ++ti) {
    if (cancel && cancel->isCanceled())
      return false;

    write_image(os, tileset->get(ti).get(), cancel);
  }

  write8(os, uint8_t(TilesetSerialFormat::LastVer));
  write_user_data(os, tileset->userData());
  write_string(os, tileset->name());

  for (tile_index ti = 0; ti < tileset->size(); ++ti) {
    if (cancel && cancel->isCanceled())
      return false;

    write_user_data(os, tileset->getTileData(ti));
  }
  return true;
}

Tileset* read_tileset(std::istream& is,
                      Sprite* sprite,
                      const bool setId,
                      TilesetSerialFormat* tilesetVer,
                      const SerialFormat serial)
{
  const ObjectId id = read32(is);
  const tileset_index ntiles = read32(is);
  const Grid grid = read_grid(is);
  auto tileset = new Tileset(sprite, grid, ntiles);
  if (setId)
    tileset->setId(id);

  for (tileset_index ti = 0; ti < ntiles; ++ti) {
    ImageRef image(read_image(is, setId));
    tileset->set(ti, image);
  }

  // Read extra version byte after tiles
  const auto ver = TilesetSerialFormat(read8(is));
  if (tilesetVer)
    *tilesetVer = ver;
  if (ver >= TilesetSerialFormat::Ver1) {
    tileset->setBaseIndex(1);

    if (ver >= TilesetSerialFormat::Ver2) {
      const UserData userData = read_user_data(is, serial);
      tileset->setUserData(userData);

      if (ver >= TilesetSerialFormat::Ver3) {
        tileset->setName(read_string(is));

        for (tileset_index ti = 0; ti < ntiles; ++ti) {
          tileset->setTileData(ti, read_user_data(is, serial));
        }
      }
    }
  }
  // Old tileset used in internal versions (this was added to recover
  // old files, maybe in a future we could remove this code)
  else {
    fix_old_tileset(tileset);
  }

  return tileset;
}

} // namespace doc
