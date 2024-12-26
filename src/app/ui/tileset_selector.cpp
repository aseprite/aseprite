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
#include "app/pref/preferences.h"
#include "doc/sprite.h"
#include "doc/tilesets.h"
#include "fmt/format.h"
#include "ui/listitem.h"
#include "ui/window.h"

namespace app {

using namespace ui;

TilesetSelector::TilesetSelector(const doc::Sprite* sprite, const TilesetSelector::Info& info)
  : m_sprite(sprite)
  , m_info(info)
{
  initTheme();

  fillControls(m_info.name, m_info.grid.tileSize(), m_info.baseIndex, m_info.matchFlags);

  if (!m_info.allowNewTileset) {
    tilesets()->deleteAllItems();
  }

  if (m_info.allowExistentTileset) {
    doc::tileset_index tsi = 0;
    for (doc::Tileset* tileset : *sprite->tilesets()) {
      if (!tileset)
        continue;

      auto item = new ListItem(fmt::format("Tileset #{0} ({1}x{2}): \"{3}\"",
                                           tsi,
                                           tileset->grid().tileSize().w,
                                           tileset->grid().tileSize().h,
                                           tileset->name()));
      tilesets()->addItem(item);

      if (m_info.tsi == tsi)
        tilesets()->setSelectedItem(item);

      ++tsi;
    }
  }

  if (m_info.enabled) {
    tilesets()->Change.connect([this]() { updateControlsState(); });
  }

  // Advanced controls
  const Preferences& pref = Preferences::instance();
  advanced()->setSelected(pref.tileset.advanced());
  advanced()->Click.connect([this]() {
    updateControlsVisibility();
    if (auto win = window())
      win->expandWindow(win->sizeHint());
  });

  updateControlsState();
  updateControlsVisibility();
}

void TilesetSelector::fillControls(const std::string& nameValue,
                                   const gfx::Size& gridSize,
                                   const int baseIndexValue,
                                   const doc::tile_flags matchFlags)
{
  name()->setText(nameValue);
  gridWidth()->setTextf("%d", gridSize.w);
  gridHeight()->setTextf("%d", gridSize.h);
  baseIndex()->setTextf("%d", baseIndexValue);
  xflip()->setSelected((matchFlags & doc::tile_f_xflip) ? true : false);
  yflip()->setSelected((matchFlags & doc::tile_f_yflip) ? true : false);
  dflip()->setSelected((matchFlags & doc::tile_f_dflip) ? true : false);
}

void TilesetSelector::updateControlsState()
{
  const doc::Tilesets* spriteTilesets = m_sprite->tilesets();

  if (m_info.enabled) {
    const int index = getSelectedItemIndex();
    const bool isNewTileset = (index == 0);
    if (isNewTileset) {
      name()->setText("");
      baseIndex()->setTextf("%d", 1);
    }
    else {
      const doc::Tileset* ts = spriteTilesets->get(index - 1);
      fillControls(ts->name(), ts->grid().tileSize(), ts->baseIndex(), ts->matchFlags());
    }

    name()->setEnabled(true);
    gridWidth()->setEnabled(isNewTileset);
    gridHeight()->setEnabled(isNewTileset);
    baseIndex()->setEnabled(true);
  }
  else {
    tilesets()->setEnabled(false);
    gridWidth()->setEnabled(false);
    gridHeight()->setEnabled(false);
    baseIndex()->setEnabled(false);
    xflip()->setEnabled(false);
    yflip()->setEnabled(false);
    dflip()->setEnabled(false);
  }
}

void TilesetSelector::updateControlsVisibility()
{
  const bool v = advanced()->isSelected();
  baseIndexLabel()->setVisible(v);
  baseIndex()->setVisible(v);
  baseIndexFiller()->setVisible(v);
  flipsLabel()->setVisible(v);
  flips()->setVisible(v);
  flipsFiller()->setVisible(v);
}

TilesetSelector::Info TilesetSelector::getInfo()
{
  int itemIndex = getSelectedItemIndex();
  Info info;
  if (itemIndex == 0) {
    gfx::Size sz(std::max(1, gridWidth()->textInt()), std::max(1, gridHeight()->textInt()));

    info.newTileset = true;
    info.grid = doc::Grid::MakeRect(sz);
  }
  else {
    info.newTileset = false;
    info.tsi = itemIndex - 1;
  }
  info.name = name()->text();

  // If we are creating a new tilemap/tileset, and the advanced
  // options are hidden, we use the default values (only in that
  // case). In other case we use the edited options (even if the
  // advanced options are hidden).
  if (m_info.allowNewTileset && m_info.newTileset && !advanced()->isSelected()) {
    info.baseIndex = 1;
    info.matchFlags = 0;
  }
  else {
    info.baseIndex = baseIndex()->textInt();
    info.matchFlags = (xflip()->isSelected() ? doc::tile_f_xflip : 0) |
                      (yflip()->isSelected() ? doc::tile_f_yflip : 0) |
                      (dflip()->isSelected() ? doc::tile_f_dflip : 0);
  }

  return info;
}

void TilesetSelector::saveAdvancedPreferences()
{
  Preferences& pref = Preferences::instance();
  pref.tileset.advanced(advanced()->isSelected());
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
