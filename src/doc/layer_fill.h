// Aseprite Document Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_FILL_H_INCLUDED
#define DOC_LAYER_FILL_H_INCLUDED
#pragma once

#include "doc/layer.h"

namespace doc {

// Fills the whole canvas with a solid color or a gradient.
// TODO to implement
class LayerFill final : public Layer {
public:
  LayerFill(Sprite* sprite);
  virtual ~LayerFill();

  // TODO associated fill data per frame/keyframe
};

} // namespace doc

#endif
