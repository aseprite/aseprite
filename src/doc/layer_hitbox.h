// Aseprite Document Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_HITBOX_H_INCLUDED
#define DOC_LAYER_HITBOX_H_INCLUDED
#pragma once

#include "doc/layer.h"

namespace doc {

// Abstract vectorial shapes, data, and events that can be associated
// to a specific frame. Generally with the purpose to define hitboxes
// to be used in a game engine.
class LayerHitbox final : public Layer {
public:
  explicit LayerHitbox(Sprite* sprite);
  virtual ~LayerHitbox();

  // TODO associated hitbox data
};

} // namespace doc

#endif
