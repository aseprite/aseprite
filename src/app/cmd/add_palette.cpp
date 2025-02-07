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
#include "doc/palette_io.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;

AddPalette::AddPalette(Sprite* sprite, Palette* pal)
  : WithSprite(sprite)
  , m_size(0)
  , m_frame(pal->frame())
{
  write_palette(m_stream, pal);
  m_size = size_t(m_stream.tellp());
}

void AddPalette::onExecute()
{
  m_stream.seekp(0);

  Sprite* sprite = this->sprite();
  Palette* pal = read_palette(m_stream);

  sprite->setPalette(pal, true);
  sprite->incrementVersion();
}

void AddPalette::onUndo()
{
  Sprite* sprite = this->sprite();

  sprite->deletePalette(m_frame);
  sprite->incrementVersion();
}

}} // namespace app::cmd
