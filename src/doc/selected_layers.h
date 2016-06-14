// Aseprite Document Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SELECTED_LAYERS_H_INCLUDED
#define DOC_SELECTED_LAYERS_H_INCLUDED
#pragma once

#include <set>

namespace doc {

  class Layer;
  class LayerGroup;

  typedef std::set<Layer*> SelectedLayers;

  void remove_children_if_parent_is_selected(SelectedLayers& layers);
  void select_all_layers(LayerGroup* group, SelectedLayers& layers);

} // namespace doc

#endif  // DOC_LAYER_LIST_H_INCLUDED
