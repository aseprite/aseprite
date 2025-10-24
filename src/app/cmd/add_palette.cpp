// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/add_palette.h"

#include "doc/palette.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;

AddPalette::AddPalette(Sprite* sprite, Palette* pal) : WithSprite(sprite), m_palette(*pal)
{
}

void AddPalette::onExecute()
{
  Sprite* sprite = this->sprite();

  sprite->setPalette(&m_palette, true);
  sprite->incrementVersion();
}

void AddPalette::onUndo()
{
  Sprite* sprite = this->sprite();

  sprite->deletePalette(m_palette.frame());
  sprite->incrementVersion();
}

}} // namespace app::cmd
