// Aseprite Document Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/layer_mask.h"

namespace doc {

LayerMask::LayerMask(Sprite* sprite) : LayerImage(ObjectType::LayerMask, sprite)
{
}

LayerMask::~LayerMask()
{
}

} // namespace doc
