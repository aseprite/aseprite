// Aseprite Document Library
// Copyright (c) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/tileset.h"

namespace doc {

Tileset::Tileset(Sprite* sprite,
                 const Grid& grid,
                 const tileset_index ntiles)
  : Object(ObjectType::Tileset)
  , m_sprite(sprite)
  , m_grid(grid)
  , m_tiles(ntiles)
{
}

void Tileset::setOrigin(const gfx::Point& pt)
{
  m_grid.origin(pt);
}

int Tileset::getMemSize() const
{
  int size = sizeof(Tileset) + m_name.size();
  for (auto& img : const_cast<Tileset*>(this)->m_tiles) {
    if (img)
      size += img->getMemSize();
  }
  return size;
}

void Tileset::resize(const tile_index ntiles)
{
  m_tiles.resize(ntiles);
}

void Tileset::setExternal(const std::string& filename,
                          const tileset_index& tsi)
{
  m_external.filename = filename;
  m_external.tileset = tsi;
}

} // namespace doc
