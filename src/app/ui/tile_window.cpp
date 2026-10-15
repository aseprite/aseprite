// Aseprite
// Copyright (C) 2019-2025  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/tile_window.h"

#include "app/doc.h"
#include "app/pref/preferences.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "app/ui/user_data_view.h"
#include "base/convert_to.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "ui/manager.h"
#include "ui/message.h"

#include <algorithm>

namespace app {

TileWindow::TileWindow(const doc::Tileset* tileset, doc::tile_index ti)
  : m_tileIndex(ti)
  , m_userData(tileset->getTileData(ti))
  , m_userDataView(Preferences::instance().tags.userDataVisibility)
{
  m_userDataView.configureAndSet(m_userData, propertiesGrid());

  tileIndex()->setTextf("%d", int(m_tileIndex) + tileset->baseIndex() - 1);

  userData()->Click.connect([this] { onToggleUserData(); });
}

bool TileWindow::show()
{
  openWindowInForeground();
  return (closer() == ok());
}

void TileWindow::onToggleUserData()
{
  m_userDataView.toggleVisibility();
  expandWindow(gfx::Size(bounds().w, sizeHint().h));
}

} // namespace app
