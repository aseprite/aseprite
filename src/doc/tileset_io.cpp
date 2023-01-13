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

// Extra BYTE with special flags to check the tileset version.  This
// field didn't exist in Aseprite v1.3-alpha3 (so read8() fails = 0)
#define TILESET_VER1     1

// Tileset has UserData now
#define TILESET_VER2     2

// Tileset name (was missing originally)
#define TILESET_VER3     3

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

bool write_tileset(std::ostream& os,
                   const Tileset* tileset,
                   CancelIO* cancel)
{
  write32(os, tileset->id());
  write32(os, tileset->size());
  write_grid(os, tileset->grid());

  for (tile_index ti=0; ti<tileset->size(); ++ti) {
    if (cancel && cancel->isCanceled())
      return false;

    write_image(os, tileset->get(ti).get(), cancel);
  }

  write8(os, TILESET_VER3);
  write_user_data(os, tileset->userData());
  write_string(os, tileset->name());
  return true;
}

Tileset* read_tileset(std::istream& is,
                      Sprite* sprite,
                      bool setId,
                      bool* isOldVersion)
{
  ObjectId id = read32(is);
  tileset_index ntiles = read32(is);
  Grid grid = read_grid(is, setId);
  auto tileset = new Tileset(sprite, grid, ntiles);
  if (setId)
    tileset->setId(id);

  for (tileset_index ti=0; ti<ntiles; ++ti) {
    ImageRef image(read_image(is, setId));
    tileset->set(ti, image);
  }

  // Read extra version byte after tiles
  uint32_t ver = read8(is);
  if (ver >= TILESET_VER1) {
    if (isOldVersion)
      *isOldVersion = false;

    tileset->setBaseIndex(1);

    if (ver >= TILESET_VER2) {
      UserData userData = read_user_data(is);
      tileset->setUserData(userData);

      if (ver >= TILESET_VER3)
        tileset->setName(read_string(is));
    }
  }
  // Old tileset used in internal versions (this was added to recover
  // old files, maybe in a future we could remove this code)
  else {
    if (isOldVersion)
      *isOldVersion = true;

    fix_old_tileset(tileset);
  }

  return tileset;
}

}
