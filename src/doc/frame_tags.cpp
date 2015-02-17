// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/frame_tags.h"

namespace doc {

FrameTags::FrameTags(Sprite* sprite)
  : m_sprite(sprite)
{
}

void FrameTags::add(FrameTag* tag)
{
  m_tags.push_back(tag);
}

void FrameTags::remove(FrameTag* tag)
{
  auto it = std::find(m_tags.begin(), m_tags.end(), tag);
  ASSERT(it != m_tags.end());
  if (it != m_tags.end())
    m_tags.erase(it);
}

} // namespace doc
