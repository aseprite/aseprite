// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remove_palette.h"

namespace app { namespace cmd {

using namespace doc;

RemovePalette::RemovePalette(Sprite* sprite, Palette* pal) : AddPalette(sprite, pal)
{
}

void RemovePalette::onExecute(Context* ctx)
{
  AddPalette::onUndo(ctx);
}

void RemovePalette::onUndo(Context* ctx)
{
  AddPalette::onRedo(ctx);
}

}} // namespace app::cmd
