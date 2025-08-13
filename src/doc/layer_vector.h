// Aseprite Document Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_VECTOR_H_INCLUDED
#define DOC_LAYER_VECTOR_H_INCLUDED
#pragma once

#include "doc/layer.h"

namespace doc {

// Dynamically renders a path/vector for each cel.
// TODO to implement
class LayerVector final : public Layer {
protected:
  LayerVector(Sprite* sprite);

public:
  virtual ~LayerVector();

  // TODO associated path/vector data per frame/keyframe
};

} // namespace doc

#endif
