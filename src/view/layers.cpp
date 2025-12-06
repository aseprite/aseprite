// Aseprite View Library
// Copyright (c) 2020-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "view/layers.h"

#include "doc/layer.h"
#include "doc/sprite.h"

namespace view {

using namespace doc;

Layer* candidate_if_layer_is_deleted(const Layer* selectedLayer, const Layer* layerToDelete)
{
  const Layer* layerToSelect = selectedLayer;

  if ((selectedLayer == layerToDelete) ||
      (selectedLayer && selectedLayer->hasAncestor(layerToDelete))) {
    Sprite* sprite = selectedLayer->sprite();
    Layer* parent = layerToDelete->parent();

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

} // namespace view
