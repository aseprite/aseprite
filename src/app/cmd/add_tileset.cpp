// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_tileset.h"

#include "doc/sprite.h"
#include "doc/subobjects_io.h"
#include "doc/tileset.h"
#include "doc/tileset_io.h"
#include "doc/tilesets.h"

namespace app {
namespace cmd {

using namespace doc;

AddTileset::AddTileset(doc::Sprite* sprite, doc::Tileset* tileset)
  : WithSprite(sprite)
  , WithTileset(tileset)
  , m_size(0)
  , m_tilesetIndex(-1)
{
}

AddTileset::AddTileset(doc::Sprite* sprite, const doc::tileset_index tsi)
  : WithSprite(sprite)
  , WithTileset(sprite->tilesets()->get(tsi))
  , m_size(0)
  , m_tilesetIndex(tsi)
{
}

void AddTileset::onExecute()
{
  Tileset* tileset = this->tileset();

  addTileset(tileset);
}

void AddTileset::onUndo()
{
  doc::Tileset* tileset = this->tileset();
  write_tileset(m_stream, tileset);
  m_size = size_t(m_stream.tellp());

  doc::Sprite* sprite = this->sprite();
  sprite->tilesets()->erase(m_tilesetIndex);

  sprite->incrementVersion();
  sprite->tilesets()->incrementVersion();

  delete tileset;
}

void AddTileset::onRedo()
{
  auto sprite = this->sprite();
  doc::Tileset* tileset = read_tileset(m_stream, sprite);

  addTileset(tileset);

  m_stream.str(std::string());
  m_stream.clear();
  m_size = 0;
}

void AddTileset::addTileset(doc::Tileset* tileset)
{
  auto sprite = this->sprite();

  if (m_tilesetIndex == -1)
    m_tilesetIndex = sprite->tilesets()->add(tileset);
  else
    sprite->tilesets()->add(m_tilesetIndex, tileset);

  sprite->incrementVersion();
  sprite->tilesets()->incrementVersion();
}

} // namespace cmd
} // namespace app
