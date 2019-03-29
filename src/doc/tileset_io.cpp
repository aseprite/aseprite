// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/tileset_io.h"

#include "base/serialization.h"
#include "doc/grid_io.h"
#include "doc/subobjects_io.h"
#include "doc/tileset.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

bool write_tileset(std::ostream& os,
                   const Tileset* tileset,
                   CancelIO* cancel)
{
  write32(os, tileset->id());
  write32(os, tileset->size());
  return write_grid(os, tileset->grid());

  // TODO save images
}

Tileset* read_tileset(std::istream& is,
                      SubObjectsFromSprite* subObjects,
                      bool setId)
{
  ObjectId id = read32(is);
  tileset_index ntiles = read32(is);
  Grid grid = read_grid(is, setId);
  auto tileset = new Tileset(subObjects->sprite(), grid, ntiles);
  if (setId)
    tileset->setId(id);
  return tileset;
}

}
