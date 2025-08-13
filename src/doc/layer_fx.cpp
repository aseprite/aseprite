// Aseprite Document Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/layer_fx.h"

namespace doc {

LayerFx::LayerFx(Sprite* sprite) : Layer(ObjectType::LayerFx, sprite)
{
}

LayerFx::~LayerFx()
{
}

void LayerFx::getCels(CelList& cels) const
{
  // TODO
}

void LayerFx::displaceFrames(frame_t fromThis, frame_t delta)
{
  // TODO
}

} // namespace doc
