// Aseprite
// Copyright (C) 2023-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_tile_data_property.h"

#include "doc/tileset.h"

namespace app { namespace cmd {

SetTileDataProperty::SetTileDataProperty(doc::Tileset* ts,
                                         doc::tile_index ti,
                                         const std::string& group,
                                         const std::string& field,
                                         doc::UserData::Variant&& newValue)
  : WithTileset(ts)
  , m_ti(ti)
  , m_group(group)
  , m_field(field)
  , m_value(std::move(newValue))
{
}

void SetTileDataProperty::onExecute()
{
  auto ts = tileset();
  auto& properties = ts->getTileData(m_ti).properties(m_group);

  auto old = properties[m_field];
  doc::set_property_value(properties, m_field, std::move(m_value));
  std::swap(m_value, old);

  ts->incrementVersion();
}

void SetTileDataProperty::onUndo()
{
  onExecute();
}

}} // namespace app::cmd
