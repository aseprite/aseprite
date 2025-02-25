// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_grid_type.h"

#include "app/doc.h"
#include "app/pref/preferences.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;

SetGridType::SetGridType(Sprite* sprite, const doc::Grid::Type type)
  : WithSprite(sprite)
  , m_oldType(sprite->gridType())
  , m_newType(type)
{
}

void SetGridType::onExecute()
{
  setGrid(m_newType);
}

void SetGridType::onUndo()
{
  setGrid(m_oldType);
}

void SetGridType::setGrid(const doc::Grid::Type type)
{
  Sprite* spr = sprite();
  spr->setGridType(type);

  Doc* doc = static_cast<Doc*>(spr->document());
  auto& docPref = Preferences::instance().document(doc);
  docPref.grid.type(type);
  spr->incrementVersion();
}

}} // namespace app::cmd
