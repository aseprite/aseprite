// Aseprite Document Library
// Copyright (c) 2016 David Capello
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

} // namespace doc
