// Aseprite Render Library
// Copyright (C) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "render/rasterize.h"

#include "base/debug.h"
#include "doc/blend_internals.h"
#include "doc/cel.h"
#include "doc/sprite.h"
#include "render/render.h"

namespace render {

void rasterize(doc::Image* dst, const doc::Cel* cel, const int x, const int y, const bool clear)
{
  ASSERT(dst);
  ASSERT(dst->pixelFormat() != IMAGE_TILEMAP);
  ASSERT(cel);

  const doc::Sprite* sprite = cel->sprite();
  ASSERT(sprite);

  if (clear)
    dst->clear(sprite->transparentColor());

  int t;
  int opacity;
  opacity = MUL_UN8(cel->opacity(), cel->layer()->opacity(), t);

  if (cel->image()->pixelFormat() == IMAGE_TILEMAP) {
    render::Render render;
    render.renderCel(dst,
                     cel,
                     sprite,
                     cel->image(),
                     cel->layer(),
                     sprite->palette(cel->frame()),
                     gfx::RectF(cel->bounds()),
                     gfx::Clip(x, y, 0, 0, dst->width(), dst->height()),
                     clear ? 255 : opacity,
                     clear ? BlendMode::NORMAL : cel->layer()->blendMode());
  }
  else {
    if (clear)
      copy_image(dst, cel->image(), x + cel->x(), y + cel->y());
    else
      composite_image(dst,
                      cel->image(),
                      sprite->palette(cel->frame()),
                      x + cel->x(),
                      y + cel->y(),
                      opacity,
                      cel->layer()->blendMode());
  }
}

void rasterize_with_cel_bounds(doc::Image* dst, const doc::Cel* cel)
{
  rasterize(dst, cel, -cel->x(), -cel->y(), true);
}

void rasterize_with_sprite_bounds(doc::Image* dst, const doc::Cel* cel)
{
  rasterize(dst, cel, 0, 0, true);
}

doc::Image* rasterize_with_cel_bounds(const doc::Cel* cel)
{
  ASSERT(cel);

  const doc::Sprite* sprite = cel->sprite();
  ASSERT(sprite);

  doc::ImageSpec spec = sprite->spec();
  spec.setWidth(cel->bounds().w);
  spec.setHeight(cel->bounds().h);

  std::unique_ptr<doc::Image> dst(doc::Image::create(spec));
  rasterize_with_cel_bounds(dst.get(), cel);
  return dst.release();
}

doc::Image* rasterize_with_sprite_bounds(const doc::Cel* cel)
{
  ASSERT(cel);

  const doc::Sprite* sprite = cel->sprite();
  ASSERT(sprite);

  std::unique_ptr<doc::Image> dst(doc::Image::create(sprite->spec()));
  rasterize_with_sprite_bounds(dst.get(), cel);
  return dst.release();
}

} // namespace render
