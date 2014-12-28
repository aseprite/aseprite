// Aseprite Render Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/doc.h"
#include "gfx/clip.h"
#include "render/render.h"

namespace render {

using namespace doc;

color_t get_sprite_pixel(const Sprite* sprite, int x, int y, frame_t frame)
{
  color_t color = 0;

  if ((x >= 0) && (y >= 0) && (x < sprite->width()) && (y < sprite->height())) {
    base::UniquePtr<Image> image(Image::create(sprite->pixelFormat(), 1, 1));

    render::Render().renderSprite(image, sprite, frame,
      gfx::Clip(0, 0, x, y, 1, 1));

    color = get_pixel(image, 0, 0);
  }

  return color;
}

} // namespace render
