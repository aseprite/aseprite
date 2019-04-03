// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/algorithm/resize_image.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "render/dithering.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"
#include "render/render.h"

#include <cmath>
#include <memory>

namespace app {

using namespace doc;

Cel* create_cel_copy(const Cel* srcCel,
                     const Sprite* dstSprite,
                     const Layer* dstLayer,
                     const frame_t dstFrame)
{
  const Image* celImage = srcCel->image();

  std::unique_ptr<Cel> dstCel(
    new Cel(dstFrame,
            ImageRef(Image::create(dstSprite->pixelFormat(),
                                   celImage->width(),
                                   celImage->height()))));

  if ((dstSprite->pixelFormat() != celImage->pixelFormat()) ||
      // If both images are indexed but with different palette, we can
      // convert the source cel to RGB first.
      (dstSprite->pixelFormat() == IMAGE_INDEXED &&
       celImage->pixelFormat() == IMAGE_INDEXED &&
       srcCel->sprite()->palette(srcCel->frame())->countDiff(
         dstSprite->palette(dstFrame), nullptr, nullptr))) {
    ImageRef tmpImage(Image::create(IMAGE_RGB, celImage->width(), celImage->height()));
    tmpImage->clear(0);

    render::convert_pixel_format(
      celImage,
      tmpImage.get(),
      IMAGE_RGB,
      render::Dithering(),
      srcCel->sprite()->rgbMap(srcCel->frame()),
      srcCel->sprite()->palette(srcCel->frame()),
      srcCel->layer()->isBackground(),
      0);

    render::convert_pixel_format(
      tmpImage.get(),
      dstCel->image(),
      IMAGE_INDEXED,
      render::Dithering(),
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

  // Resize a referecen cel to a non-reference layer
  if (srcCel->layer()->isReference() && !dstLayer->isReference()) {
    gfx::RectF srcBounds = srcCel->boundsF();

    std::unique_ptr<Cel> dstCel2(
      new Cel(dstFrame,
              ImageRef(Image::create(dstSprite->pixelFormat(),
                                     std::ceil(srcBounds.w),
                                     std::ceil(srcBounds.h)))));
    algorithm::resize_image(
      dstCel->image(), dstCel2->image(),
      algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR,
      nullptr, nullptr, 0);

    dstCel.reset(dstCel2.release());
    dstCel->setPosition(gfx::Point(srcBounds.origin()));
  }
  // Copy original cel bounds
  else {
    if (srcCel->layer() &&
        srcCel->layer()->isReference()) {
      dstCel->setBoundsF(srcCel->boundsF());
    }
    else {
      dstCel->setPosition(srcCel->position());
    }
  }

  dstCel->setOpacity(srcCel->opacity());
  dstCel->data()->setUserData(srcCel->data()->userData());

  return dstCel.release();
}

} // namespace app
