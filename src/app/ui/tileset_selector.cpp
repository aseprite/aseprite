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
                                 const TilesetSelector::Info& info)
{
  initTheme();

  name()->setText(info.name);
  gridWidth()->setTextf("%d", info.grid.tileSize().w);
  gridHeight()->setTextf("%d", info.grid.tileSize().h);
  baseIndex()->setTextf("%d", info.baseIndex);

  doc::tileset_index tsi = 0;
  for (doc::Tileset* tileset : *sprite->tilesets()) {
    if (!tileset)
      continue;

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

  if (info.enabled) {
    tilesets()->Change.connect(
      [this, sprite]() {
        int index = tilesets()->getSelectedItemIndex();
        bool editable = (index == 0);

        if (index == 0) {
          name()->setText("");
          baseIndex()->setTextf("%d", 1);
        }
        else {
          doc::Tileset* ts = sprite->tilesets()->get(index-1);
          doc::Grid grid = ts->grid();
          name()->setText(ts->name());
          gridWidth()->setTextf("%d", grid.tileSize().w);
          gridHeight()->setTextf("%d", grid.tileSize().h);
          baseIndex()->setTextf("%d", ts->baseIndex());
        }

        name()->setEnabled(editable);
        gridWidth()->setEnabled(editable);
        gridHeight()->setEnabled(editable);
        baseIndex()->setEnabled(editable);
      });
  }
  else {
    tilesets()->setEnabled(false);
    gridWidth()->setEnabled(false);
    gridHeight()->setEnabled(false);
  }
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
  info.name = name()->text();
  info.baseIndex = baseIndex()->textInt();
  return info;
}

} // namespace app
