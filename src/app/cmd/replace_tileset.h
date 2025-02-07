// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REPLACE_TILESET_H_INCLUDED
#define APP_CMD_REPLACE_TILESET_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/tileset.h"

#include <sstream>

namespace app { namespace cmd {

class ReplaceTileset : public Cmd,
                       public WithSprite {
public:
  ReplaceTileset(doc::Sprite* sprite, const doc::tileset_index tsi, doc::Tileset* newTileset);

protected:
  void onExecute() override;
  void onUndo() override { onExecute(); }
  void onRedo() override { onExecute(); }
  size_t onMemSize() const override { return sizeof(*this) + m_size; }

private:
  void replaceTileset(doc::Tileset* newTileset);

  doc::tileset_index m_tsi;
  doc::Tileset* m_newTileset;
  std::stringstream m_stream;
  size_t m_size = 0;
};

}} // namespace app::cmd

#endif
