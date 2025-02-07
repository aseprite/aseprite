// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ADD_TILESET_H_INCLUDED
#define APP_CMD_ADD_TILESET_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "app/cmd/with_tileset.h"
#include "doc/tile.h"

#include <sstream>

namespace doc {
class Tileset;
}

namespace app { namespace cmd {

class AddTileset : public Cmd,
                   public WithSprite,
                   public WithTileset {
public:
  AddTileset(doc::Sprite* sprite, doc::Tileset* tileset);
  AddTileset(doc::Sprite* sprite, const doc::tileset_index tsi);

  doc::tileset_index tilesetIndex() const { return m_tilesetIndex; }

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_size; }

private:
  void addTileset(doc::Tileset* tileset);

  size_t m_size;
  std::stringstream m_stream;
  doc::tileset_index m_tilesetIndex;
};

}} // namespace app::cmd

#endif
