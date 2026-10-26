// Aseprite Document Library
// Copyright (c) 2019-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/tilesets.h"

#include <limits>
#include <stdexcept>

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

void Tilesets::set(const tileset_index tsi, Tileset* tileset)
{
  auto newSize = Array::size_type(tsi);
  if (newSize == std::numeric_limits<Array::size_type>::max())
    throw std::runtime_error("not enough space for given tileset index");
  ++newSize;

  if (newSize > m_tilesets.size())
    m_tilesets.resize(newSize, nullptr);

  m_tilesets[tsi] = tileset;
}

void Tilesets::add(const tileset_index tsi, Tileset* tileset)
{
  if (tsi >= m_tilesets.size()) {
    m_tilesets.push_back(tileset);
  }
  else {
    m_tilesets.insert(m_tilesets.begin() + tsi, tileset);
    // Update tileset indexes of the affected tilemaps. We have to shift the indexes
    // for all the tilemaps pointing to a tileset index equals or greater than the added one.
    shiftTilesetIndexes(tileset->sprite(), tsi, 1);
  }
}

void Tilesets::erase(const tileset_index tsi)
{
  // When tsi is the last one, other tilemaps tilesets
  // indexes are not affected.
  if (tsi == size() - 1) {
    m_tilesets.erase(--m_tilesets.end());
  }
  else {
    auto ts = m_tilesets[tsi];
    m_tilesets.erase(m_tilesets.begin() + tsi);
    // Update tileset indexes of the affected tilemaps. We have to shift the indexes
    // for all the tilemaps pointing to a tileset index greater than the deleted one.
    shiftTilesetIndexes(ts->sprite(), tsi + 1, -1);
  }
}

} // namespace doc
