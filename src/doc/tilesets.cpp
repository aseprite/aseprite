// Aseprite Document Library
// Copyright (c) 2019-2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/tilesets.h"

namespace doc {

Tilesets::Tilesets() : Object(ObjectType::Tilesets)
{
}

Tilesets::~Tilesets()
{
  for (auto tileset : m_tilesets)
    delete tileset;
}

int Tilesets::getMemSize() const
{
  int size = sizeof(Tilesets);
  for (auto tileset : m_tilesets) {
    if (tileset)
      size += tileset->getMemSize();
  }
  return size;
}

tileset_index Tilesets::add(Tileset* tileset)
{
  // The tileset can be nullptr to add an empty slot in the Tilesets
  // (and align tileset indexes).
  m_tilesets.push_back(tileset);
  return tileset_index(m_tilesets.size() - 1);
}

} // namespace doc
