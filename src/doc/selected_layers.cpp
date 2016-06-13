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

} // namespace doc
