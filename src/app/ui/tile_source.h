// Aseprite
// Copyright (c) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TILE_SOURCE_H_INCLUDED
#define APP_UI_TILE_SOURCE_H_INCLUDED
#pragma once

#include "doc/tile.h"
#include "gfx/point.h"

namespace app {

class ITileSource {
public:
  virtual ~ITileSource() {}
  virtual doc::tile_t getTileByPosition(const gfx::Point& pos) = 0;
};

} // namespace app

#endif
