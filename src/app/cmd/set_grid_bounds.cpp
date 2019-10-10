// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_grid_bounds.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "app/doc_observer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

SetGridBounds::SetGridBounds(Sprite* sprite, const gfx::Rect& bounds)
  : WithSprite(sprite)
  , m_oldBounds(sprite->gridBounds())
  , m_newBounds(bounds)
{
}

void SetGridBounds::onExecute()
{
  Sprite* spr = sprite();
  spr->setGridBounds(m_newBounds);
  spr->incrementVersion();
}

void SetGridBounds::onUndo()
{
  Sprite* spr = sprite();
  spr->setGridBounds(m_oldBounds);
  spr->incrementVersion();
}

void SetGridBounds::onFireNotifications()
{
  Sprite* sprite = this->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  doc->notify_observers<DocEvent&>(&DocObserver::onSpriteGridBoundsChanged, ev);
}

} // namespace cmd
} // namespace app
