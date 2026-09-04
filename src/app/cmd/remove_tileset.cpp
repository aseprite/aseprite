// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remove_tileset.h"

#include "doc/cel.h"
#include "doc/layer.h"

namespace app { namespace cmd {

using namespace doc;

RemoveTileset::RemoveTileset(Sprite* sprite, const tileset_index si) : AddTileset(sprite, si)
{
}

void RemoveTileset::onExecute(Context* ctx)
{
  AddTileset::onUndo(ctx);
}

void RemoveTileset::onUndo(Context* ctx)
{
  AddTileset::onRedo(ctx);
}

void RemoveTileset::onRedo(Context* ctx)
{
  AddTileset::onUndo(ctx);
}

}} // namespace app::cmd
