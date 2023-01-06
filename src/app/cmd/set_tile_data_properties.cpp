// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tile_data_properties.h"

#include "doc/tileset.h"

namespace app {
namespace cmd {

SetTileDataProperties::SetTileDataProperties(
  doc::Tileset* ts,
  doc::tile_index ti,
  const std::string& group,
  doc::UserData::Properties&& newProperties)
  : WithTileset(ts)
  , m_ti(ti)
  , m_group(group)
  , m_oldProperties(ts->getTileData(ti).properties(group))
  , m_newProperties(std::move(newProperties))
{
}

void SetTileDataProperties::onExecute()
{
  auto ts = tileset();
  ts->getTileData(m_ti).properties(m_group) = m_newProperties;
  ts->incrementVersion();
}

void SetTileDataProperties::onUndo()
{
  auto ts = tileset();
  ts->getTileData(m_ti).properties(m_group) = m_oldProperties;
  ts->incrementVersion();
}

} // namespace cmd
} // namespace app
