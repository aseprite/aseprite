// Aseprite
// Copyright (C) 2019-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/add_tile.h"

#include "app/doc.h"
#include "doc/sprite.h"
#include "doc/tileset.h"
#include "doc/tilesets.h"

namespace app { namespace cmd {

AddTile::AddTile(doc::Tileset* tileset, const doc::ImageRef& image, const doc::UserData& userData)
  : WithTileset(tileset)
  , m_tileIndex(doc::notile)
  , m_imageRef(image)
  , m_userData(userData)
{
}

AddTile::AddTile(doc::Tileset* tileset, const doc::tile_index ti)
  : WithTileset(tileset)
  , m_tileIndex(ti)
  , m_imageRef(tileset->get(ti))
  , m_userData(tileset->getTileData(ti))
{
}

void AddTile::onExecute()
{
  doc::Tileset* tileset = this->tileset();
  ASSERT(tileset);

  if (m_tileIndex != doc::notile) {
    tileset->sprite()->incrementVersion();
    tileset->incrementVersion();
  }
  else {
    addTile(tileset, m_imageRef, m_userData);
  }
}

void AddTile::onUndo()
{
  doc::Tileset* tileset = this->tileset();
  ASSERT(tileset);

  ASSERT(m_imageRef);
  m_suspendedImage.suspend(m_imageRef);
  m_imageRef.reset();

  tileset->erase(m_tileIndex);

  tileset->sprite()->incrementVersion();
  tileset->incrementVersion();
}

void AddTile::onRedo()
{
  doc::Tileset* tileset = this->tileset();

  ASSERT(!m_imageRef);
  m_imageRef = m_suspendedImage.restore();
  ASSERT(m_imageRef);

  addTile(tileset, m_imageRef, m_userData);
}

void AddTile::onFireNotifications()
{
  doc::Tileset* tileset = this->tileset();

  // Notify that the tileset's changed
  static_cast<Doc*>(tileset->sprite()->document())->notifyTilesetChanged(tileset);
}

void AddTile::addTile(doc::Tileset* tileset,
                      const doc::ImageRef& image,
                      const doc::UserData& userData)
{
  if (m_tileIndex == doc::notile)
    m_tileIndex = tileset->add(image, userData);
  else
    tileset->insert(m_tileIndex, image, userData);

  tileset->sprite()->incrementVersion();
  tileset->incrementVersion();
}

}} // namespace app::cmd
