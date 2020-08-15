// Aseprite Document Library
// Copyright (C) 2019-2020  Igara Studio S.A.
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
  write_grid(os, tileset->grid());

  for (tile_index ti=0; ti<tileset->size(); ++ti) {
    if (cancel && cancel->isCanceled())
      return false;

    write_image(os, tileset->get(ti).get(), cancel);
  }

  return true;
}

Tileset* read_tileset(std::istream& is,
                      Sprite* sprite,
                      bool setId)
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

  return tileset;
}

}
