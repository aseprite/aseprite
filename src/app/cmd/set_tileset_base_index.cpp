// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_tileset_base_index.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/tileset.h"

namespace app { namespace cmd {

SetTilesetBaseIndex::SetTilesetBaseIndex(Tileset* tileset, int baseIndex)
  : WithTileset(tileset)
  , m_oldBaseIndex(tileset->baseIndex())
  , m_newBaseIndex(baseIndex)
{
}

void SetTilesetBaseIndex::onExecute()
{
  auto ts = tileset();
  ts->setBaseIndex(m_newBaseIndex);
  ts->incrementVersion();
  ts->sprite()->incrementVersion();
}

void SetTilesetBaseIndex::onUndo()
{
  auto ts = tileset();
  ts->setBaseIndex(m_oldBaseIndex);
  ts->incrementVersion();
  ts->sprite()->incrementVersion();
}

}} // namespace app::cmd
