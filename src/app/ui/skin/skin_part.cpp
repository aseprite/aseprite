// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/skin_part.h"

#include "she/surface.h"

namespace app {
namespace skin {

SkinPart::SkinPart()
{
}

SkinPart::~SkinPart()
{
  clear();
}

void SkinPart::clear()
{
  for (Bitmaps::iterator it = m_bitmaps.begin(), end = m_bitmaps.end();
       it != end; ++it) {
    ASSERT(*it != NULL);

    (*it)->dispose();
    *it = NULL;
  }
}

void SkinPart::setBitmap(std::size_t index, she::Surface* bitmap)
{
  if (index >= m_bitmaps.size())
    m_bitmaps.resize(index+1, NULL);

  m_bitmaps[index] = bitmap;
}

gfx::Size SkinPart::size() const
{
  if (!m_bitmaps.empty())
    return gfx::Size(m_bitmaps[0]->width(),
                     m_bitmaps[0]->height());
  else
    return gfx::Size(0, 0);
}

} // namespace skin
} // namespace app
