// Aseprite
// Copyright (c) 2019-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TILESET_SELECTOR_H_INCLUDED
#define APP_UI_TILESET_SELECTOR_H_INCLUDED
#pragma once

#include "doc/grid.h"
#include "doc/tile.h"

#include <string>

#include "tileset_selector.xml.h"

namespace doc {
  class Sprite;
  class Tileset;
  class Tilesets;
}

namespace app {

  class TilesetSelector : public app::gen::TilesetSelector {
  public:
    struct Info {
      // Enables/disables the tileset selector combobox and fields to
      // edit the tileset data.
      bool enabled = true;

      // When false, removes the "New Tileset" option from the tileset
      // selector combobox.
      bool allowNewTileset = true;

      // When false, we only can select a "New Tileset" option or
      // change the tileset properties.
      bool allowExistentTileset = true;

      // Output members that are set when TilesetSelector.getInfo()
      // is called.
      bool newTileset = true;  // Set to true when the selected item is "New Tileset"
      std::string name;
      doc::Grid grid;
      int baseIndex = 1;
      doc::tile_flags matchFlags = 0;
      doc::tileset_index tsi = -1;
    };

    TilesetSelector(const doc::Sprite* sprite,
                    const TilesetSelector::Info& info);

    // Returns the data of this widget according to the user input
    Info getInfo();

    void saveAdvancedPreferences();

  private:
    void fillControls(const std::string& name,
                      const gfx::Size& gridSize,
                      const int baseIndex,
                      const doc::tile_flags matchFlags);
    void updateControlsState();
    void updateControlsVisibility();

    // Returns the selected item index as if the combobox always has the "New Tileset"
    // as its first item.
    int getSelectedItemIndex();

    const doc::Sprite* m_sprite;

    // Holds the information used to create this widget
    const TilesetSelector::Info m_info;
  };

} // namespace app

#endif
