// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2018  David Capello
// Copyright (C) 2016  Carlo Caputo
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/blend_mode.h"
#include "doc/cel.h"
#include "doc/conversion_to_surface.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "os/surface.h"
#include "os/system.h"
#include "render/render.h"

namespace app {
namespace thumb {

os::Surface* get_cel_thumbnail(const doc::Cel* cel,
                               const gfx::Size& fitInSize)
{
  gfx::Size newSize;

  if (cel->bounds().w > fitInSize.w ||
      cel->bounds().h > fitInSize.h)
    newSize = gfx::Rect(cel->bounds()).fitIn(gfx::Rect(fitInSize)).size();
  else
    newSize = cel->bounds().size();

  if (newSize.w < 1 ||
      newSize.h < 1)
    return nullptr;

  doc::ImageRef thumbnailImage(
    doc::Image::create(
      doc::IMAGE_RGB, newSize.w, newSize.h));

  render::Render render;
  render::Projection proj(cel->sprite()->pixelRatio(),
                          render::Zoom(newSize.w, cel->image()->width()));
  render.setProjection(proj);

  const doc::Palette* palette = cel->sprite()->palette(cel->frame());
  render.renderCel(
    thumbnailImage.get(),
    cel->sprite(),
    cel->image(),
    cel->layer(),
    palette,
    gfx::Rect(gfx::Point(0, 0), cel->bounds().size()),
    gfx::Clip(gfx::Rect(gfx::Point(0, 0), newSize)),
    255, doc::BlendMode::NORMAL);

  if (os::Surface* thumbnail = os::instance()->createRgbaSurface(
        thumbnailImage->width(),
        thumbnailImage->height())) {
    convert_image_to_surface(
      thumbnailImage.get(), palette, thumbnail,
      0, 0, 0, 0, thumbnailImage->width(), thumbnailImage->height());
    return thumbnail;
  }
  else
    return nullptr;
}

} // thumb
} // app
