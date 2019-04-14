// Aseprite
// Copyright (c) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TILESET_SELECTOR_H_INCLUDED
#define APP_UI_TILESET_SELECTOR_H_INCLUDED
#pragma once

#include "doc/grid.h"
#include "doc/tile.h"

#include "tileset_selector.xml.h"

namespace doc {
  class Sprite;
  class Tileset;
}

namespace app {

  class TilesetSelector : public app::gen::TilesetSelector {
  public:
    struct Info {
      bool newTileset = true;
      doc::Grid grid;
      doc::tileset_index tsi = 0;
    };

    TilesetSelector(const doc::Sprite* sprite,
                    const TilesetSelector::Info& info);

    Info getInfo();

  protected:
  };

} // namespace app

#endif
