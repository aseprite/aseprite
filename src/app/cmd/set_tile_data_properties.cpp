// Aseprite
// Copyright (C) 2023-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_tile_data_properties.h"

#include "doc/tileset.h"

namespace app { namespace cmd {

SetTileDataProperties::SetTileDataProperties(doc::Tileset* ts,
                                             doc::tile_index ti,
                                             const std::string& group,
                                             doc::UserData::Properties&& newProperties)
  : WithTileset(ts)
  , m_ti(ti)
  , m_group(group)
  , m_properties(std::move(newProperties))
{
}

void SetTileDataProperties::onExecute()
{
  auto ts = tileset();
  auto& properties = ts->getTileData(m_ti).properties(m_group);

  auto old = properties;
  properties = m_properties;
  std::swap(m_properties, old);

  ts->incrementVersion();
}

void SetTileDataProperties::onUndo()
{
  onExecute();
}

}} // namespace app::cmd
