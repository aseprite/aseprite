// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_sprite_size.h"

#include "app/doc.h"
#include "app/doc_event.h"
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
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  doc->notify_observers<DocEvent&>(&DocObserver::onSpriteSizeChanged, ev);
}

} // namespace cmd
} // namespace app
