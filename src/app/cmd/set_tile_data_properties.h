// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TILE_DATA_PROPERTIES_H_INCLUDED
#define APP_CMD_SET_TILE_DATA_PROPERTIES_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tileset.h"
#include "doc/tile.h"
#include "doc/user_data.h"

namespace app {
namespace cmd {

  class SetTileDataProperties : public Cmd,
                                public WithTileset {
  public:
    SetTileDataProperties(
      doc::Tileset* ts,
      doc::tile_index ti,
      const std::string& group,
      doc::UserData::Properties&& newProperties);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);     // TODO + properties size
    }

  private:
    doc::tile_index m_ti;
    std::string m_group;
    doc::UserData::Properties m_oldProperties;
    doc::UserData::Properties m_newProperties;
  };

} // namespace cmd
} // namespace app

#endif
