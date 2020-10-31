// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/tileset_selector.h"

#include "doc/sprite.h"
#include "doc/tilesets.h"
#include "fmt/format.h"
#include "ui/listitem.h"
#include "ui/window.h"

namespace app {

using namespace ui;

TilesetSelector::TilesetSelector(const doc::Sprite* sprite,
                                 const TilesetSelector::Info& info)
{
  initTheme();

  gridWidth()->setTextf("%d", info.grid.tileSize().w);
  gridHeight()->setTextf("%d", info.grid.tileSize().h);
  baseIndex()->setTextf("%d", info.baseIndex);

  doc::tileset_index tsi = 0;
  for (doc::Tileset* tileset : *sprite->tilesets()) {
    auto item = new ListItem(
      fmt::format("Tileset #{0} ({1}x{2}): \"{3}\"",
                  tsi,
                  tileset->grid().tileSize().w,
                  tileset->grid().tileSize().w,
                  tileset->name()));
    tilesets()->addItem(item);

    if (info.tsi == tsi)
      tilesets()->setSelectedItem(item);

    ++tsi;
  }

  if (!info.enabled) {
    tilesets()->setEnabled(false);
    gridWidth()->setEnabled(false);
    gridHeight()->setEnabled(false);
  }

  tilesets()->Change.connect(
    [this, sprite]() {
      int index = tilesets()->getSelectedItemIndex();
      gridOptions()->setVisible(index == 0);
      gridOptions()->setVisible(index == 0);
      baseIndex()->setTextf(
        "%d", (index == 0 ? 1: sprite->tilesets()->get(index-1)->baseIndex()));
      this->window()->layout();
    });
}

TilesetSelector::Info TilesetSelector::getInfo()
{
  int itemIndex = tilesets()->getSelectedItemIndex();
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
  info.baseIndex = baseIndex()->textInt();
  return info;
}

} // namespace app
