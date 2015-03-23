// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remap_colors.h"

#include "doc/image.h"
#include "doc/remap.h"
#include "doc/sprite.h"
#include "doc/image_bits.h"

namespace app {
namespace cmd {

using namespace doc;

RemapColors::RemapColors(Sprite* sprite, const Remap& remap)
  : WithSprite(sprite)
  , m_remap(remap)
{
}

void RemapColors::onExecute()
{
  Sprite* spr = sprite();
  if (spr->pixelFormat() == IMAGE_INDEXED)
    spr->remapImages(0, spr->lastFrame(), m_remap);
}

void RemapColors::onUndo()
{
  Sprite* spr = this->sprite();
  if (spr->pixelFormat() == IMAGE_INDEXED)
    spr->remapImages(0, spr->lastFrame(), m_remap.invert());
}

} // namespace cmd
} // namespace app
