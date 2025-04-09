// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_grid_type.h"

#include "base/exception.h"
#include "app/doc.h"
#include "app/i18n/strings.h"
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

} // namespace cmd

std::string grid_type_to_string(const doc::Grid::Type t)
{
  if (t == doc::Grid::Type::Orthogonal)
    return app::Strings::grid_settings_type_orthogonal();
  if (t == doc::Grid::Type::IsometricSharedEdges)
    return app::Strings::grid_settings_type_isometric_shared();
  if (t == doc::Grid::Type::IsometricThickEdges)
    return app::Strings::grid_settings_type_isometric_thick();

  throw base::Exception("Invalid grid type index: " + std::to_string(int(t)));
}

doc::Grid::Type string_to_grid_type(const std::string& s)
{
  if (s == app::Strings::grid_settings_type_orthogonal())
    return doc::Grid::Type::Orthogonal;
  if (s == app::Strings::grid_settings_type_isometric_shared())
    return doc::Grid::Type::IsometricSharedEdges;
  if (s == app::Strings::grid_settings_type_isometric_thick())
    return doc::Grid::Type::IsometricThickEdges;

  throw base::Exception("Invalid string for grid type: " + s);
}

} // namespace app
