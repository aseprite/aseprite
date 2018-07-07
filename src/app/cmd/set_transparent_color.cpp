// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_transparent_color.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetTransparentColor::SetTransparentColor(Sprite* sprite, color_t newMask)
  : WithSprite(sprite)
  , m_oldMaskColor(sprite->transparentColor())
  , m_newMaskColor(newMask)
{
}

void SetTransparentColor::onExecute()
{
  Sprite* spr = sprite();
  spr->setTransparentColor(m_newMaskColor);
  spr->incrementVersion();
}

void SetTransparentColor::onUndo()
{
  Sprite* spr = sprite();
  spr->setTransparentColor(m_oldMaskColor);
  spr->incrementVersion();
}

void SetTransparentColor::onFireNotifications()
{
  Sprite* sprite = this->sprite();
  auto doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  doc->notify_observers<DocEvent&>(&DocObserver::onSpriteTransparentColorChanged, ev);
}

} // namespace cmd
} // namespace app
