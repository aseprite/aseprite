// Aseprite
// Copyright (c) 2019-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/resize_image.h"

#include "app/cmd/replace_image.h"
#include "app/cmd/set_cel_bounds.h"
#include "app/cmd/set_cel_position.h"
#include "app/tx.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace app {

doc::Image* resize_image(const doc::Image* image,
                         const gfx::SizeF& scale,
                         const doc::algorithm::ResizeMethod method,
                         const Palette* pal,
                         const RgbMap* rgbmap)
{
  doc::ImageSpec spec = image->spec();
  spec.setWidth(std::max(1, int(std::round(scale.w * image->width()))));
  spec.setHeight(std::max(1, int(std::round(scale.h * image->height()))));
  std::unique_ptr<doc::Image> newImage(doc::Image::create(spec));
  newImage->setMaskColor(image->maskColor());

  doc::algorithm::fixup_image_transparent_colors(newImage.get());
  doc::algorithm::resize_image(image, newImage.get(), method, pal, rgbmap, newImage->maskColor());

  return newImage.release();
}

void resize_cel_image(Tx& tx,
                      doc::Cel* cel,
                      const gfx::SizeF& scale,
                      const doc::algorithm::ResizeMethod method,
                      const gfx::PointF& pivot)
{
  // Get cel's image
  doc::Image* image = cel->image();
  if (image && !cel->link()) {
    doc::Sprite* sprite = cel->sprite();

    // Resize the cel bounds only if it's from a reference layer
    if (cel->layer()->isReference()) {
      gfx::RectF newBounds = cel->boundsF();
      newBounds.offset(pivot - gfx::PointF(scale.w * pivot.x, scale.h * pivot.y));
      newBounds.w *= scale.w;
      newBounds.h *= scale.h;
      tx(new cmd::SetCelBoundsF(cel, newBounds));
    }
    else {
      // Change cel location
      const int x = cel->x() + pivot.x - scale.w * pivot.x;
      const int y = cel->y() + pivot.y - scale.h * pivot.y;
      if (cel->x() != x || cel->y() != y)
        tx(new cmd::SetCelPosition(cel, x, y));

      // Resize the image
      const int w = std::max(1, int(scale.w * image->width()));
      const int h = std::max(1, int(scale.h * image->height()));
      doc::ImageRef newImage(
        doc::Image::create(image->pixelFormat(), std::max(1, w), std::max(1, h)));
      newImage->setMaskColor(image->maskColor());

      doc::algorithm::fixup_image_transparent_colors(image);
      doc::algorithm::resize_image(
        image,
        newImage.get(),
        method,
        sprite->palette(cel->frame()),
        sprite->rgbMap(cel->frame()),
        (cel->layer()->isBackground() ? -1 : sprite->transparentColor()));

      tx(new cmd::ReplaceImage(sprite, cel->imageRef(), newImage));
    }
  }
}

} // namespace app
