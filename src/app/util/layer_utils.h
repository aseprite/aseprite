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
  class Sprite;
}

namespace app {

  class Editor;

  // True if the active layer is locked (itself or its hierarchy),
  // also, it sends a tip to the user 'Layer ... is locked'
  bool layer_is_locked(Editor* editor);

  std::string get_layer_path(const doc::Layer* layer);

  doc::Layer* copy_layer(doc::Layer* layer);
  doc::Layer* copy_layer_with_sprite(doc::Layer* layer, doc::Sprite* sprite);

} // namespace app

#endif
