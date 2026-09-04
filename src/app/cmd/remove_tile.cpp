// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remove_tile.h"

#include "doc/cel.h"
#include "doc/layer.h"

namespace app { namespace cmd {

using namespace doc;

RemoveTile::RemoveTile(Tileset* tileset, const tile_index ti) : AddTile(tileset, ti)
{
}

void RemoveTile::onExecute(Context* ctx)
{
  AddTile::onUndo(ctx);
}

void RemoveTile::onUndo(Context* ctx)
{
  AddTile::onRedo(ctx);
}

void RemoveTile::onRedo(Context* ctx)
{
  AddTile::onUndo(ctx);
}

}} // namespace app::cmd
