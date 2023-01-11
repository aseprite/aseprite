// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_layer_tileset.h"

#include "app/doc.h"
#include "doc/layer_tilemap.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetLayerTileset::SetLayerTileset(doc::LayerTilemap* layer,
                                 doc::tileset_index newTsi)
  : WithLayer(layer)
  , m_oldTsi(layer->tilesetIndex())
  , m_newTsi(newTsi)
{
}

void SetLayerTileset::onExecute()
{
  auto layer = static_cast<LayerTilemap*>(this->layer());
  layer->setTilesetIndex(m_newTsi);
  layer->incrementVersion();
}

void SetLayerTileset::onUndo()
{
  auto layer = static_cast<LayerTilemap*>(this->layer());
  layer->setTilesetIndex(m_oldTsi);
  layer->incrementVersion();
}

} // namespace cmd
} // namespace app
