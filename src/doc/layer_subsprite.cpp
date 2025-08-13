// Aseprite Document Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/layer_subsprite.h"

namespace doc {

LayerSubsprite::LayerSubsprite(Sprite* sprite) : Layer(ObjectType::LayerSubsprite, sprite)
{
}

LayerSubsprite::~LayerSubsprite()
{
}

void LayerSubsprite::getCels(CelList& cels) const
{
  // TODO
}

void LayerSubsprite::displaceFrames(frame_t fromThis, frame_t delta)
{
  // TODO
}

} // namespace doc
