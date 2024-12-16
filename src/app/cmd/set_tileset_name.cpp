// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_tileset_name.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/tileset.h"

namespace app { namespace cmd {

SetTilesetName::SetTilesetName(Tileset* tileset, const std::string& name)
  : WithTileset(tileset)
  , m_oldName(tileset->name())
  , m_newName(name)
{
}

void SetTilesetName::onExecute()
{
  auto ts = tileset();
  ts->setName(m_newName);
  ts->incrementVersion();
  ts->sprite()->incrementVersion();
}

void SetTilesetName::onUndo()
{
  auto ts = tileset();
  ts->setName(m_oldName);
  ts->incrementVersion();
  ts->sprite()->incrementVersion();
}

}} // namespace app::cmd
