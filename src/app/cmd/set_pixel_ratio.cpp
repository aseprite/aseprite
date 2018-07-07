// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_pixel_ratio.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "app/doc_observer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

SetPixelRatio::SetPixelRatio(Sprite* sprite, PixelRatio pixelRatio)
  : WithSprite(sprite)
  , m_oldRatio(sprite->pixelRatio())
  , m_newRatio(pixelRatio)
{
}

void SetPixelRatio::onExecute()
{
  Sprite* spr = sprite();
  spr->setPixelRatio(m_newRatio);
  spr->incrementVersion();
}

void SetPixelRatio::onUndo()
{
  Sprite* spr = sprite();
  spr->setPixelRatio(m_oldRatio);
  spr->incrementVersion();
}

void SetPixelRatio::onFireNotifications()
{
  Sprite* sprite = this->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  doc->notify_observers<DocEvent&>(&DocObserver::onSpritePixelRatioChanged, ev);
}

} // namespace cmd
} // namespace app
