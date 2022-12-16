// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ADD_TILE_H_INCLUDED
#define APP_CMD_ADD_TILE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "app/cmd/with_tileset.h"
#include "doc/image_ref.h"
#include "doc/tile.h"
#include "doc/user_data.h"

#include <sstream>

namespace doc {
  class Tileset;
}

namespace app {
namespace cmd {

  class AddTile : public Cmd
                , public WithTileset
                , public WithImage {
  public:
    AddTile(doc::Tileset* tileset,
            const doc::ImageRef& image,
            const doc::UserData& userData = UserData());
    AddTile(doc::Tileset* tileset,
            const doc::tile_index ti);

    doc::tile_index tileIndex() const { return m_tileIndex; }

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      // TODO add m_userData size
      return sizeof(*this) + m_size;
    }

  private:
    void addTile(doc::Tileset* tileset,
                 const doc::ImageRef& image,
                 const doc::UserData& userData);

    size_t m_size;
    std::stringstream m_stream;
    doc::tile_index m_tileIndex;
    doc::ImageRef m_imageRef;
    doc::UserData m_userData;
  };

} // namespace cmd
} // namespace app

#endif
