// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMAP_TILESET_H_INCLUDED
#define APP_CMD_REMAP_TILESET_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tileset.h"
#include "doc/remap.h"

namespace app { namespace cmd {
using namespace doc;

class RemapTileset : public Cmd,
                     public WithTileset {
public:
  RemapTileset(Tileset* tileset, const Remap& remap);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_remap.getMemSize(); }

private:
  void applyRemap(Tileset* tileset, const Remap& remap);

  Remap m_remap;
};

}} // namespace app::cmd

#endif
