// Aseprite Document Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/layer_text.h"

namespace doc {

LayerText::LayerText(Sprite* sprite) : Layer(ObjectType::LayerText, sprite)
{
}

LayerText::~LayerText()
{
}

void LayerText::getCels(CelList& cels) const
{
  // TODO
}

void LayerText::displaceFrames(frame_t fromThis, frame_t delta)
{
  // TODO
}

} // namespace doc
