// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TILE_DATA_PROPERTY_H_INCLUDED
#define APP_CMD_SET_TILE_DATA_PROPERTY_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tileset.h"
#include "doc/tile.h"
#include "doc/user_data.h"

namespace app {
namespace cmd {

  class SetTileDataProperty : public Cmd,
                              public WithTileset {
  public:
    SetTileDataProperty(
      doc::Tileset* ts,
      doc::tile_index ti,
      const std::string& group,
      const std::string& field,
      doc::UserData::Variant&& newValue);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);     // TODO + variant size
    }

  private:
    doc::tile_index m_ti;
    std::string m_group;
    std::string m_field;
    doc::UserData::Variant m_oldValue;
    doc::UserData::Variant m_newValue;
  };

} // namespace cmd
} // namespace app

#endif
