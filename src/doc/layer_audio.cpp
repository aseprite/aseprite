// Aseprite Document Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/layer_audio.h"

namespace doc {

LayerAudio::LayerAudio(Sprite* sprite) : Layer(ObjectType::LayerAudio, sprite)
{
}

LayerAudio::~LayerAudio()
{
}

void LayerAudio::getCels(CelList& cels) const
{
  // TODO
}

void LayerAudio::displaceFrames(frame_t fromThis, frame_t delta)
{
  // TODO
}

} // namespace doc
