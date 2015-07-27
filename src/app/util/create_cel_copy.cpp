// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/sprite.h"
#include "render/render.h"

namespace app {

using namespace doc;

Cel* create_cel_copy(const Cel* srcCel,
                     const Sprite* dstSprite,
                     const frame_t dstFrame)
{
  base::UniquePtr<Cel> dstCel(
    new Cel(dstFrame,
            ImageRef(Image::create(dstSprite->pixelFormat(),
                                   srcCel->image()->width(),
                                   srcCel->image()->height()))));

  render::composite_image(
    dstCel->image(), srcCel->image(),
    srcCel->sprite()->palette(srcCel->frame()), // TODO add dst palette
    0, 0, 255, BlendMode::SRC);

  dstCel->setPosition(srcCel->position());
  dstCel->setOpacity(srcCel->opacity());

  return dstCel.release();
}

} // namespace app
