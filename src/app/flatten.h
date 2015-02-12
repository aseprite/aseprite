// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_FLATTEN_H_INCLUDED
#define APP_FLATTEN_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "doc/frame.h"

namespace doc {
  class Sprite;
  class Layer;
  class LayerImage;
}

namespace app {

  // Returns a new layer with the given layer at "srcLayer" rendered
  // frame by frame from "frmin" to "frmax" (inclusive).  The routine
  // flattens all children of "srcLayer" to an unique output layer.
  //
  // Note: The layer is not added to the given sprite, but is related to
  // it, so you'll be able to add the flatten layer only into the given
  // sprite.
  LayerImage* create_flatten_layer_copy(Sprite* dstSprite, const Layer* srcLayer,
                                        const gfx::Rect& bounds,
                                        frame_t frmin, frame_t frmax);

} // namespace app

#endif
