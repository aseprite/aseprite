// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/layer_utils.h"

#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

using namespace doc;

Layer* candidate_if_layer_is_deleted(
  const Layer* selectedLayer,
  const Layer* layerToDelete)
{
  const Layer* layerToSelect = selectedLayer;

  if ((selectedLayer == layerToDelete) ||
      (selectedLayer &&
       selectedLayer->hasAncestor(layerToDelete))) {
    Sprite* sprite = selectedLayer->sprite();
    LayerGroup* parent = layerToDelete->parent();

    // Select previous layer, or next layer, or the parent (if it is
    // not the main layer of sprite set).
    if (layerToDelete->getPrevious())
      layerToSelect = layerToDelete->getPrevious();
    else if (layerToDelete->getNext())
      layerToSelect = layerToDelete->getNext();
    else if (parent != sprite->root())
      layerToSelect = parent;
  }

  return const_cast<Layer*>(layerToSelect);
}

} // namespace app
