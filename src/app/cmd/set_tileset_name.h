// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TILESET_NAME_H_INCLUDED
#define APP_CMD_SET_TILESET_NAME_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tileset.h"

#include <string>

namespace app { namespace cmd {
using namespace doc;

class SetTilesetName : public Cmd,
                       public WithTileset {
public:
  SetTilesetName(Tileset* tileset, const std::string& name);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  std::string m_oldName;
  std::string m_newName;
};

}} // namespace app::cmd

#endif
