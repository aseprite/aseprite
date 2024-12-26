// Aseprite Document Library
// Copyright (c) 2019-2020  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_TILEMAP_H_INCLUDED
#define DOC_LAYER_TILEMAP_H_INCLUDED
#pragma once

#include "doc/layer.h"
#include "doc/tile.h"
#include "doc/tileset.h"

namespace doc {

class LayerTilemap : public LayerImage {
public:
  explicit LayerTilemap(Sprite* sprite, const tileset_index tsi);
  ~LayerTilemap();

  Grid grid() const override;

  // Returns the tileset of this layer. New automatically-created
  // tiles should be stored into this tileset, and all tiles in the
  // layer should share the same Grid spec.
  Tileset* tileset() const { return m_tileset; }
  tileset_index tilesetIndex() const { return m_tilesetIndex; }

  void setTilesetIndex(tileset_index tsi);

private:
  Tileset* m_tileset;
  tileset_index m_tilesetIndex;
};

} // namespace doc

#endif
