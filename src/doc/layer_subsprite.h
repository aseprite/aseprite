// Aseprite Document Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_SUBSPRITE_H_INCLUDED
#define DOC_LAYER_SUBSPRITE_H_INCLUDED
#pragma once

#include "doc/layer.h"

namespace doc {

// Includes an subsprite (external or embedded)
// TODO to implement
class LayerSubsprite final : public Layer {
protected:
  LayerSubsprite(Sprite* sprite);

public:
  virtual ~LayerSubsprite();

  // TODO associated subsprite and bounds/rotation/transformation per frame/keyframe

  void getCels(CelList& cels) const override;
  void displaceFrames(frame_t fromThis, frame_t delta) override;
};

} // namespace doc

#endif
