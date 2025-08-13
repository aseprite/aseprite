// Aseprite Document Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_TEXT_H_INCLUDED
#define DOC_LAYER_TEXT_H_INCLUDED
#pragma once

#include "doc/layer.h"

namespace doc {

// Dynamically renders a text for each cel.
// TODO to implement
class LayerText final : public Layer {
protected:
  LayerText(Sprite* sprite);

public:
  virtual ~LayerText();

  // TODO associated text/font/location per frame/keyframe
};

} // namespace doc

#endif
