// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TILE_DATA_H_INCLUDED
#define APP_CMD_SET_TILE_DATA_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tileset.h"
#include "doc/tile.h"
#include "doc/user_data.h"

namespace app {
namespace cmd {

  class SetTileData : public Cmd,
                      public WithTileset {
  public:
    SetTileData(doc::Tileset* ts,
                doc::tile_index ti,
                const doc::UserData& ud);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);     // TODO + properties size
    }

  private:
    doc::tile_index m_ti;
    doc::UserData m_oldUserData;
    doc::UserData m_newUserData;
  };

} // namespace cmd
} // namespace app

#endif
