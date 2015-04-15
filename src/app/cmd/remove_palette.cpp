// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remove_palette.h"

namespace app {
namespace cmd {

using namespace doc;

RemovePalette::RemovePalette(Sprite* sprite, Palette* pal)
  : AddPalette(sprite, pal)
{
}

void RemovePalette::onExecute()
{
  AddPalette::onUndo();
}

void RemovePalette::onUndo()
{
  AddPalette::onRedo();
}

} // namespace cmd
} // namespace app
