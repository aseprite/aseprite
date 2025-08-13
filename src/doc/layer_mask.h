// Aseprite Document Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_MASK_H_INCLUDED
#define DOC_LAYER_MASK_H_INCLUDED
#pragma once

#include "doc/layer.h"

namespace doc {

// Applies Cel images as a mask for the next layer.
// TODO to implement
class LayerMask final : public LayerImage {
protected:
  LayerMask(Sprite* sprite);

public:
  virtual ~LayerMask();
};

} // namespace doc

#endif
