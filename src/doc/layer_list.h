// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_LIST_H_INCLUDED
#define DOC_LAYER_LIST_H_INCLUDED
#pragma once

#include <vector>

namespace doc {

class Layer;

typedef std::vector<Layer*> LayerList;
typedef int layer_t;

layer_t find_layer_index(const LayerList& layers, const Layer* layer);
bool are_layers_adjacent(const LayerList& layers);

} // namespace doc

#endif // DOC_LAYER_LIST_H_INCLUDED
