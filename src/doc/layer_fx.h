// Aseprite Document Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_FX_H_INCLUDED
#define DOC_LAYER_FX_H_INCLUDED
#pragma once

#include "doc/layer.h"

namespace doc {

// Applies an FX to the next non-FX Layer
// TODO to implement
class LayerFx final : public Layer {
protected:
  LayerFx(Sprite* sprite);

public:
  virtual ~LayerFx();

  // TODO associated FX data per frame/keyframe

  void getCels(CelList& cels) const override;
  void displaceFrames(frame_t fromThis, frame_t delta) override;
};

} // namespace doc

#endif
