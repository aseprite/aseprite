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
#include "app/pref/preferences.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;

SetGridBounds::SetGridBounds(Sprite* sprite, const gfx::Rect& bounds)
  : WithSprite(sprite)
  , m_oldBounds(sprite->gridBounds())
  , m_newBounds(bounds)
{
}

void SetGridBounds::onExecute()
{
  setGrid(m_newBounds);
}

void SetGridBounds::onUndo()
{
  setGrid(m_oldBounds);
}

void SetGridBounds::setGrid(const gfx::Rect& grid)
{
  Sprite* spr = sprite();
  spr->setGridBounds(grid);

  Doc* doc = static_cast<Doc*>(spr->document());
  auto& docPref = Preferences::instance().document(doc);
  docPref.grid.bounds(grid);

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

}} // namespace app::cmd
