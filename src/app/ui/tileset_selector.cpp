// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/tileset_selector.h"

#include "app/i18n/strings.h"
#include "doc/sprite.h"
#include "doc/tilesets.h"
#include "fmt/format.h"
#include "ui/listitem.h"
#include "ui/window.h"

namespace app {

using namespace ui;

TilesetSelector::TilesetSelector(const doc::Sprite* sprite,
                                 const TilesetSelector::Info& info) : m_info(info)
{
  initTheme();

  name()->setText(m_info.name);
  gridWidth()->setTextf("%d", m_info.grid.tileSize().w);
  gridHeight()->setTextf("%d", m_info.grid.tileSize().h);
  baseIndex()->setTextf("%d", m_info.baseIndex);

  if (!m_info.allowNewTileset) {
    tilesets()->deleteAllItems();
  }

  doc::tileset_index tsi = 0;
  for (doc::Tileset* tileset : *sprite->tilesets()) {
    if (!tileset)
      continue;

    auto item = new ListItem(
      fmt::format("Tileset #{0} ({1}x{2}): \"{3}\"",
                  tsi,
                  tileset->grid().tileSize().w,
                  tileset->grid().tileSize().h,
                  tileset->name()));
    tilesets()->addItem(item);

    if (m_info.tsi == tsi)
      tilesets()->setSelectedItem(item);

    ++tsi;
  }

  if (m_info.enabled) {
    tilesets()->Change.connect(
      [this, sprite]() {
        updateControlsState(sprite->tilesets());
      });
  }
  updateControlsState(sprite->tilesets());
}

void TilesetSelector::updateControlsState(const doc::Tilesets* spriteTilesets)
{
  if (m_info.enabled) {
    int index = getSelectedItemIndex();
    bool isNewTileset = (index == 0);
    if (isNewTileset) {
      name()->setText("");
      baseIndex()->setTextf("%d", 1);
    }
    else {
      doc::Tileset* ts = spriteTilesets->get(index-1);
      doc::Grid grid = ts->grid();
      name()->setText(ts->name());
      gridWidth()->setTextf("%d", grid.tileSize().w);
      gridHeight()->setTextf("%d", grid.tileSize().h);
      baseIndex()->setTextf("%d", ts->baseIndex());
    }

    name()->setEnabled(isNewTileset || !m_info.allowNewTileset);
    gridWidth()->setEnabled(isNewTileset);
    gridHeight()->setEnabled(isNewTileset);
    baseIndex()->setEnabled(isNewTileset);
  }
  else {
    tilesets()->setEnabled(false);
    gridWidth()->setEnabled(false);
    gridHeight()->setEnabled(false);
  }
}

TilesetSelector::Info TilesetSelector::getInfo()
{
  int itemIndex = getSelectedItemIndex();
  Info info;
  if (itemIndex == 0) {
    gfx::Size sz(std::max(1, gridWidth()->textInt()),
                 std::max(1, gridHeight()->textInt()));

    info.newTileset = true;
    info.grid = doc::Grid::MakeRect(sz);
  }
  else {
    info.newTileset = false;
    info.tsi = itemIndex-1;
  }
  info.name = name()->text();
  info.baseIndex = baseIndex()->textInt();
  return info;
}

int TilesetSelector::getSelectedItemIndex()
{
  int index = tilesets()->getSelectedItemIndex();
  // We have to compensate the selected index when allowNewTileset is false,
  // because the "New Tileset" item was removed.
  if (!m_info.allowNewTileset) {
    index++;
  }
  return index;
}

} // namespace app
