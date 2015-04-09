// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_sprite_size.h"

#include "app/document.h"
#include "doc/document_event.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetSpriteSize::SetSpriteSize(Sprite* sprite, int newWidth, int newHeight)
  : WithSprite(sprite)
  , m_oldWidth(sprite->width())
  , m_oldHeight(sprite->height())
  , m_newWidth(newWidth)
  , m_newHeight(newHeight)
{
  ASSERT(m_newWidth > 0);
  ASSERT(m_newHeight > 0);
}

void SetSpriteSize::onExecute()
{
  Sprite* spr = sprite();
  spr->setSize(m_newWidth, m_newHeight);
  spr->incrementVersion();
}

void SetSpriteSize::onUndo()
{
  Sprite* spr = sprite();
  spr->setSize(m_oldWidth, m_oldHeight);
  spr->incrementVersion();
}

void SetSpriteSize::onFireNotifications()
{
  Sprite* sprite = this->sprite();
  DocumentEvent ev(sprite->document());
  ev.sprite(sprite);
  sprite->document()->notifyObservers<DocumentEvent&>(&DocumentObserver::onSpriteSizeChanged, ev);
}

} // namespace cmd
} // namespace app
