// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_transparent_color.h"

#include "app/document.h"
#include "doc/document_event.h"
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
  DocumentEvent ev(sprite->document());
  ev.sprite(sprite);
  sprite->document()->notifyObservers<DocumentEvent&>(&DocumentObserver::onSpriteTransparentColorChanged, ev);
}

} // namespace cmd
} // namespace app
