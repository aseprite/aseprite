// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remove_slice.h"

namespace app { namespace cmd {

using namespace doc;

RemoveSlice::RemoveSlice(Sprite* sprite, Slice* slice) : AddSlice(sprite, slice)
{
}

void RemoveSlice::onExecute(Context* ctx)
{
  AddSlice::onUndo(ctx);
}

void RemoveSlice::onUndo(Context* ctx)
{
  AddSlice::onRedo(ctx);
}

void RemoveSlice::onRedo(Context* ctx)
{
  AddSlice::onUndo(ctx);
}

}} // namespace app::cmd
