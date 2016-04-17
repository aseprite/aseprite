// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/image_wrap.h"

#include "app/cmd/copy_region.h"
#include "app/script/sprite_wrap.h"
#include "app/transaction.h"
#include "doc/image.h"

namespace app {

ImageWrap::ImageWrap(SpriteWrap* sprite, doc::Image* img)
  : m_sprite(sprite)
  , m_image(img)
  , m_backup(nullptr)
{
}

ImageWrap::~ImageWrap()
{
}

void ImageWrap::commit()
{
  if (m_modifiedRegion.isEmpty())
    return;

  sprite()->transaction().execute(
    new cmd::CopyRegion(m_image,
                        m_backup.get(),
                        m_modifiedRegion, 0, 0,
                        true));

  m_backup.reset(nullptr);
  m_modifiedRegion.clear();
}

void ImageWrap::modifyRegion(const gfx::Region& rgn)
{
  if (!m_backup)
    m_backup.reset(doc::Image::createCopy(m_image));

  m_modifiedRegion |= rgn;
}

} // namespace app
