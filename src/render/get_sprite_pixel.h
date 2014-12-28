// Aseprite Render Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_GET_SPRITE_PIXEL_H_INCLUDED
#define RENDER_GET_SPRITE_PIXEL_H_INCLUDED
#pragma once

#include "doc/frame.h"

namespace doc {
  class Sprite;
}

namespace render {
  using namespace doc;

  // Gets a pixel from the sprite in the specified position. If in the
  // specified coordinates there're background this routine will
  // return the 0 color (the mask-color).
  color_t get_sprite_pixel(const Sprite* sprite, int x, int y, frame_t frame);

} // namespace render

#endif
