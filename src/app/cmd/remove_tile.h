// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMOVE_TILE_H_INCLUDED
#define APP_CMD_REMOVE_TILE_H_INCLUDED
#pragma once

#include "app/cmd/add_tile.h"

namespace app { namespace cmd {
using namespace doc;

class RemoveTile : public AddTile {
public:
  RemoveTile(Tileset* tileset, const tile_index ti);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
};

}} // namespace app::cmd

#endif
