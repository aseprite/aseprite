// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_palette.h"

#include "doc/sprite.h"
#include "doc/palette.h"
#include "doc/palette_io.h"

namespace app {
namespace cmd {

using namespace doc;

AddPalette::AddPalette(Sprite* sprite, Palette* pal)
  : WithSprite(sprite)
  , WithPalette(pal)
{
}

void AddPalette::onExecute()
{
  Sprite* sprite = this->sprite();
  Palette* palette = this->palette();

  sprite->setPalette(palette, true);
  sprite->incrementVersion();
}

void AddPalette::onUndo()
{
  Sprite* sprite = this->sprite();
  Palette* pal = this->palette();

  write_palette(m_stream, pal);

  sprite->deletePalette(pal);
  sprite->incrementVersion();
  delete pal;
}

void AddPalette::onRedo()
{
  Sprite* sprite = this->sprite();
  Palette* pal = read_palette(m_stream);

  sprite->setPalette(pal, true);
  sprite->incrementVersion();

  m_stream.str(std::string());
  m_stream.clear();
}

} // namespace cmd
} // namespace app
