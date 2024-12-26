// Aseprite Document Library
// Copyright (c) 2016-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/layer.h"
#include "doc/layer_list.h"

#include <algorithm>

namespace doc {

layer_t find_layer_index(const LayerList& layers, const Layer* layer)
{
  auto it = std::find(layers.begin(), layers.end(), layer);
  if (it != layers.end())
    return it - layers.begin();
  else
    return 0;
}

bool are_layers_adjacent(const LayerList& layers)
{
  layer_t count = 0;
  Layer* prev = nullptr;
  for (auto layer : layers) {
    if (prev && prev != layer->getPrevious())
      break;
    prev = layer;
    ++count;
  }
  if (count == layer_t(layers.size()))
    return true;

  count = 0;
  prev = nullptr;
  for (auto layer : layers) {
    if (prev && prev != layer->getNext())
      break;
    prev = layer;
    ++count;
  }
  if (count == layer_t(layers.size()))
    return true;

  return false;
}

} // namespace doc
