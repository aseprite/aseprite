// Aseprite Render Library
// Copyright (c) 2019 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
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

color_t get_sprite_pixel(const Sprite* sprite,
                         const double x,
                         const double y,
                         const frame_t frame,
                         const Projection& proj,
                         const bool newBlend)
{
  color_t color = 0;

  if ((x >= 0.0) && (x < sprite->width()) &&
      (y >= 0.0) && (y < sprite->height())) {
    std::unique_ptr<Image> image(Image::create(sprite->pixelFormat(), 1, 1));

    render::Render render;
    render.setNewBlend(newBlend);
    render.setRefLayersVisiblity(true);
    render.setProjection(proj);
    render.renderSprite(
      image.get(), sprite, frame,
      gfx::ClipF(0, 0, proj.applyX(x), proj.applyY(y), 1, 1));

    color = get_pixel(image.get(), 0, 0);
  }

  return color;
}

} // namespace render
