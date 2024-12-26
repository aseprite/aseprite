// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TILESET_MATCH_FLAGS_H_INCLUDED
#define APP_CMD_SET_TILESET_MATCH_FLAGS_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tileset.h"
#include "doc/tile.h"

namespace app { namespace cmd {
using namespace doc;

class SetTilesetMatchFlags : public Cmd,
                             public WithTileset {
public:
  SetTilesetMatchFlags(Tileset* tileset, const tile_flags matchFlags);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  tile_flags m_oldMatchFlags;
  tile_flags m_newMatchFlags;
};

}} // namespace app::cmd

#endif
