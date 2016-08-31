// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "render/quantization.h"
#include "render/render.h"

namespace app {

using namespace doc;

Cel* create_cel_copy(const Cel* srcCel,
                     const Sprite* dstSprite,
                     const frame_t dstFrame)
{
  const Image* celImage = srcCel->image();

  base::UniquePtr<Cel> dstCel(
    new Cel(dstFrame,
            ImageRef(Image::create(dstSprite->pixelFormat(),
                                   celImage->width(),
                                   celImage->height()))));

  // If both images are indexed but with different palette, we can
  // convert the source cel to RGB first.
  if (dstSprite->pixelFormat() == IMAGE_INDEXED &&
      celImage->pixelFormat() == IMAGE_INDEXED &&
      srcCel->sprite()->palette(srcCel->frame())->countDiff(
        dstSprite->palette(dstFrame), nullptr, nullptr)) {
    ImageRef tmpImage(Image::create(IMAGE_RGB, celImage->width(), celImage->height()));
    tmpImage->clear(0);

    render::convert_pixel_format(
      celImage,
      tmpImage.get(),
      IMAGE_RGB,
      DitheringMethod::NONE,
      srcCel->sprite()->rgbMap(srcCel->frame()),
      srcCel->sprite()->palette(srcCel->frame()),
      srcCel->layer()->isBackground(),
      0);

    render::convert_pixel_format(
      tmpImage.get(),
      dstCel->image(),
      IMAGE_INDEXED,
      DitheringMethod::NONE,
      dstSprite->rgbMap(dstFrame),
      dstSprite->palette(dstFrame),
      srcCel->layer()->isBackground(),
      dstSprite->transparentColor());
  }
  else {
    render::composite_image(
      dstCel->image(),
      celImage,
      srcCel->sprite()->palette(srcCel->frame()),
      0, 0, 255, BlendMode::SRC);
  }

  dstCel->setPosition(srcCel->position());
  dstCel->setOpacity(srcCel->opacity());
  dstCel->data()->setUserData(srcCel->data()->userData());

  return dstCel.release();
}

} // namespace app
