// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tile_data.h"

#include "doc/tileset.h"

namespace app {
namespace cmd {

SetTileData::SetTileData(doc::Tileset* ts,
                         doc::tile_index ti,
                         const doc::UserData& ud)
  : WithTileset(ts)
  , m_ti(ti)
  , m_oldUserData(ts->getTileData(ti))
  , m_newUserData(ud)
{
}

void SetTileData::onExecute()
{
  auto ts = tileset();
  ts->setTileData(m_ti, m_newUserData);
  ts->incrementVersion();
}

void SetTileData::onUndo()
{
  auto ts = tileset();
  ts->setTileData(m_ti, m_oldUserData);
  ts->incrementVersion();
}

} // namespace cmd
} // namespace app
