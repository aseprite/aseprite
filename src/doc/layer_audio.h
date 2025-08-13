// Aseprite Document Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_AUDIO_H_INCLUDED
#define DOC_LAYER_AUDIO_H_INCLUDED
#pragma once

#include "doc/layer.h"

namespace doc {

// Starts playing audio at specific times.
class LayerAudio final : public Layer {
public:
  explicit LayerAudio(Sprite* sprite);
  virtual ~LayerAudio();

  // TODO associated audio data from/to frame

  void getCels(CelList& cels) const override;
  void displaceFrames(frame_t fromThis, frame_t delta) override;
};

} // namespace doc

#endif
