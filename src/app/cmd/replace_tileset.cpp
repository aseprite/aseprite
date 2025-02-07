// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/replace_tileset.h"

#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "doc/tileset.h"
#include "doc/tileset_io.h"
#include "doc/tilesets.h"

namespace app { namespace cmd {

using namespace doc;

ReplaceTileset::ReplaceTileset(doc::Sprite* sprite,
                               const doc::tileset_index tsi,
                               doc::Tileset* newTileset)
  : WithSprite(sprite)
  , m_tsi(tsi)
  , m_newTileset(newTileset)
{
}

void ReplaceTileset::onExecute()
{
  Sprite* spr = sprite();
  doc::Tileset* actualTileset = spr->tilesets()->get(m_tsi);
  doc::Tileset* restoreTileset;
  if (m_newTileset) {
    restoreTileset = m_newTileset;
    m_newTileset = nullptr;
  }
  else {
    restoreTileset = doc::read_tileset(m_stream, spr, true);
  }

  m_stream.str(std::string());
  m_stream.clear();
  doc::write_tileset(m_stream, actualTileset);
  m_size = size_t(m_stream.tellp());

  replaceTileset(restoreTileset);
  delete actualTileset;
}

void ReplaceTileset::replaceTileset(Tileset* newTileset)
{
  Sprite* spr = sprite();

  for (Layer* lay : spr->allLayers()) {
    if (lay->isTilemap() && static_cast<LayerTilemap*>(lay)->tilesetIndex() == m_tsi) {
      lay->incrementVersion();
    }
  }

  spr->replaceTileset(m_tsi, newTileset);
  spr->tilesets()->incrementVersion();
  spr->incrementVersion();
}

}} // namespace app::cmd
