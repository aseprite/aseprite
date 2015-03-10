// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/frame_tags.h"

#include "doc/frame_tag.h"

#include <algorithm>

namespace doc {

FrameTags::FrameTags(Sprite* sprite)
  : m_sprite(sprite)
{
}

void FrameTags::add(FrameTag* tag)
{
  auto it = begin(), end = this->end();
  for (; it != end; ++it) {
    if ((*it)->fromFrame() > tag->fromFrame())
      break;
  }
  m_tags.insert(it, tag);
  tag->setOwner(this);
}

void FrameTags::remove(FrameTag* tag)
{
  auto it = std::find(m_tags.begin(), m_tags.end(), tag);
  ASSERT(it != m_tags.end());
  if (it != m_tags.end())
    m_tags.erase(it);

  tag->setOwner(nullptr);
}

FrameTag* FrameTags::getByName(const std::string& name) const
{
  for (FrameTag* tag : *this) {
    if (tag->name() == name)
      return tag;
  }
  return nullptr;
}

FrameTag* FrameTags::getById(ObjectId id) const
{
  for (FrameTag* tag : *this) {
    if (tag->id() == id)
      return tag;
  }
  return nullptr;
}

} // namespace doc
