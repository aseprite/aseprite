// Aseprite
// Copyright (C) 2019-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/add_tileset.h"

#include "doc/sprite.h"
#include "doc/tileset.h"
#include "doc/tilesets.h"

namespace app { namespace cmd {

using namespace doc;

AddTileset::AddTileset(doc::Sprite* sprite, doc::Tileset* tileset)
  : WithSprite(sprite)
  , WithTileset(tileset)
  , m_tilesetIndex(-1)
{
}

AddTileset::AddTileset(doc::Sprite* sprite, const doc::tileset_index tsi)
  : WithSprite(sprite)
  , WithTileset(sprite->tilesets()->get(tsi))
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
  m_suspendedTileset.suspend(tileset);

  doc::Sprite* sprite = this->sprite();
  sprite->tilesets()->erase(m_tilesetIndex);

  sprite->incrementVersion();
  sprite->tilesets()->incrementVersion();
}

void AddTileset::onRedo()
{
  doc::Tileset* tileset = m_suspendedTileset.restore();

  addTileset(tileset);
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

}} // namespace app::cmd
