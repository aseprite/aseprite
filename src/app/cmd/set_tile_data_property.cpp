// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tile_data_property.h"

#include "doc/tileset.h"

namespace app {
namespace cmd {

SetTileDataProperty::SetTileDataProperty(
  doc::Tileset* ts,
  doc::tile_index ti,
  const std::string& group,
  const std::string& field,
  doc::UserData::Variant&& newValue)
  : WithTileset(ts)
  , m_ti(ti)
  , m_group(group)
  , m_field(field)
  , m_oldValue(ts->getTileData(m_ti).properties(m_group)[m_field])
  , m_newValue(std::move(newValue))
{
}

void SetTileDataProperty::onExecute()
{
  auto ts = tileset();
  auto& properties = ts->getTileData(m_ti).properties(m_group);

  if (m_newValue.type() == USER_DATA_PROPERTY_TYPE_NULLPTR) {
    auto it = properties.find(m_field);
    if (it != properties.end())
      properties.erase(it);
  }
  else {
    properties[m_field] = m_newValue;
  }

  ts->incrementVersion();
}

void SetTileDataProperty::onUndo()
{
  auto ts = tileset();
  auto& properties = ts->getTileData(m_ti).properties(m_group);

  if (m_oldValue.type() == USER_DATA_PROPERTY_TYPE_NULLPTR) {
    auto it = properties.find(m_field);
    if (it != properties.end())
      properties.erase(it);
  }
  else {
    properties[m_field] = m_oldValue;
  }

  ts->incrementVersion();
}

} // namespace cmd
} // namespace app
