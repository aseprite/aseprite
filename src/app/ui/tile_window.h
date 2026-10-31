// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TILE_WINDOW_H_INCLUDED
#define APP_UI_TILE_WINDOW_H_INCLUDED
#pragma once

#include "app/ui/color_button.h"
#include "app/ui/expr_entry.h"
#include "app/ui/user_data_view.h"
#include "doc/anidir.h"
#include "doc/tile.h"
#include "doc/user_data.h"

#include "tile_properties.xml.h"

namespace doc {
class Tileset;
} // namespace doc

namespace app {

class TileWindow : protected app::gen::TileProperties {
public:
  TileWindow(const doc::Tileset* tileset, doc::tile_index tileIndex);

  bool show();

  const doc::UserData& userDataValue() const { return m_userDataView.userData(); }

private:
  void onToggleUserData();

  doc::tile_index m_tileIndex;
  doc::UserData m_userData;
  UserDataView m_userDataView;
};

} // namespace app

#endif
