// Aseprite Document Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/selected_layers.h"

#include "doc/layer.h"
#include "doc/sprite.h"

namespace doc {

bool SelectedLayers::contains(Layer* layer) const
{
  return m_set.find(layer) != m_set.end();
}

bool SelectedLayers::hasSameParent() const
{
  Layer* parent = nullptr;
  for (auto layer : *this) {
    if (parent) {
      if (layer->parent() != parent)
        return false;
    }
    else
      parent = layer->parent();
  }
  return true;
}

LayerList SelectedLayers::toLayerList() const
{
  LayerList output;

  if (empty())
    return output;

  for (Layer* layer = (*begin())->sprite()->firstBrowsableLayer();
       layer != nullptr;
       layer = layer->getNextInWholeHierarchy()) {
    if (contains(layer))
      output.push_back(layer);
  }

  return output;
}

void SelectedLayers::removeChildrenIfParentIsSelected()
{
  SelectedLayers removeThese;

  for (Layer* child : *this) {
    Layer* parent = child->parent();
    while (parent) {
      if (contains(parent)) {
        removeThese.insert(child);
        break;
      }
      parent = parent->parent();
    }
  }

  for (Layer* child : removeThese)
    erase(child);
}

void SelectedLayers::selectAllLayers(LayerGroup* group)
{
  for (Layer* layer : group->layers()) {
    if (layer->isGroup())
      selectAllLayers(static_cast<LayerGroup*>(layer));
    insert(layer);
  }
}

void SelectedLayers::displace(layer_t layerDelta)
{
  const SelectedLayers original = *this;
  clear();

  for (auto it : original) {
    Layer* layer = it;

    if (layerDelta > 0) {
      for (layer_t i=0; layer && i<layerDelta; ++i)
        layer = layer->getNextInWholeHierarchy();
    }
    else if (layerDelta < 0) {
      for (layer_t i=0; layer && i>layerDelta; --i) {
        layer = layer->getPreviousInWholeHierarchy();
      }
    }

    if (layer)
      insert(layer);
  }
}

} // namespace doc
