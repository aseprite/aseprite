// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_LAYER_UTILS_H_INCLUDED
#define APP_LAYER_UTILS_H_INCLUDED
#pragma once

#include <string>

namespace doc {
class Layer;
}

namespace app {

class Editor;

// Calculates a possible candidate to be selected in case that we
// have a specific "selectedLayer" and are going to delete the given
// "layerToDelete".
doc::Layer* candidate_if_layer_is_deleted(const doc::Layer* selectedLayer,
                                          const doc::Layer* layerToDelete);

// True if the active layer is locked (itself or its hierarchy),
// also, it sends a tip to the user 'Layer ... is locked'
bool layer_is_locked(Editor* editor);

std::string get_layer_path(const doc::Layer* layer);

} // namespace app

#endif
