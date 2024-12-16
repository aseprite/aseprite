// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remap_colors.h"

#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/remap.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;

RemapColors::RemapColors(Sprite* sprite, const Remap& remap) : WithSprite(sprite), m_remap(remap)
{
}

void RemapColors::onExecute()
{
  Sprite* spr = sprite();
  if (spr->pixelFormat() == IMAGE_INDEXED) {
    spr->remapImages(m_remap);
    incrementVersions(spr);
  }
}

void RemapColors::onUndo()
{
  Sprite* spr = this->sprite();
  if (spr->pixelFormat() == IMAGE_INDEXED) {
    spr->remapImages(m_remap.invert());
    incrementVersions(spr);
  }
}

void RemapColors::incrementVersions(Sprite* spr)
{
  for (const Cel* cel : spr->uniqueCels())
    cel->image()->incrementVersion();
}

}} // namespace app::cmd
