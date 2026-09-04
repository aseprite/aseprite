// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMAP_TILEMAPS_H_INCLUDED
#define APP_CMD_REMAP_TILEMAPS_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tileset.h"
#include "doc/remap.h"

namespace app { namespace cmd {
using namespace doc;

class RemapTilemaps : public Cmd,
                      public WithTileset {
public:
  RemapTilemaps(Tileset* tileset, const Remap& remap);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this) + m_remap.getMemSize(); }

private:
  void remapTileset(Tileset* tileset, const Remap& remap);
  void incrementVersions(Tileset* tileset);

  Remap m_remap;
};

}} // namespace app::cmd

#endif
