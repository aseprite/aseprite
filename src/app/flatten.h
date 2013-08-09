/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_FLATTEN_H_INCLUDED
#define APP_FLATTEN_H_INCLUDED

#include "gfx/rect.h"
#include "raster/frame_number.h"

namespace raster {
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
                                        FrameNumber frmin, FrameNumber frmax);

} // namespace app

#endif
