// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_LAYER_BOUNDARIES_H_INCLUDED
#define APP_LAYER_BOUNDARIES_H_INCLUDED
#pragma once

#include "doc/frame.h"

namespace doc {
  class Layer;
}

namespace app {

  enum SelectLayerBoundariesOp {
    REPLACE, ADD, SUBTRACT, INTERSECT
  };

  void select_layer_boundaries(doc::Layer* layer,
                               const doc::frame_t frame,
                               const SelectLayerBoundariesOp op);

} // namespace app

#endif
