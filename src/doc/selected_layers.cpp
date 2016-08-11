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

void remove_children_if_parent_is_selected(SelectedLayers& layers)
{
  SelectedLayers removeThese;

  for (Layer* child : layers) {
    Layer* parent = child->parent();
    while (parent) {
      if (layers.find(parent) != layers.end()) {
        removeThese.insert(child);
        break;
      }
      parent = parent->parent();
    }
  }

  for (Layer* child : removeThese) {
    layers.erase(child);
  }
}

void select_all_layers(LayerGroup* group, SelectedLayers& layers)
{
  for (Layer* layer : group->layers()) {
    if (layer->isGroup())
      select_all_layers(static_cast<LayerGroup*>(layer), layers);
    layers.insert(layer);
  }
}

LayerList convert_selected_layers_into_layer_list(const SelectedLayers& layers)
{
  LayerList output;

  if (layers.empty())
    return output;

  for (Layer* layer = (*layers.begin())->sprite()->firstBrowsableLayer();
       layer != nullptr;
       layer = layer->getNextInWholeHierarchy()) {
    if (layers.find(layer) != layers.end()) {
      output.push_back(layer);
    }
  }

  return output;
}

void displace_selected_layers(SelectedLayers& layers, layer_t layerDelta)
{
  const SelectedLayers original = layers;
  layers.clear();

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
      layers.insert(layer);
  }
}

} // namespace doc
