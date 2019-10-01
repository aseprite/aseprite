// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/tags.h"

#include "base/debug.h"
#include "doc/tag.h"

#include <algorithm>

namespace doc {

Tags::Tags(Sprite* sprite)
  : m_sprite(sprite)
{
}

Tags::~Tags()
{
  for (Tag* tag : m_tags) {
    tag->setOwner(nullptr);
    delete tag;
  }
}

void Tags::add(Tag* tag)
{
  auto it = begin(), end = this->end();
  for (; it != end; ++it) {
    if ((*it)->fromFrame() > tag->fromFrame() ||
        ((*it)->fromFrame() == tag->fromFrame() &&
         (*it)->toFrame() < tag->toFrame()))
      break;
  }
  m_tags.insert(it, tag);
  tag->setOwner(this);
}

void Tags::remove(Tag* tag)
{
  auto it = std::find(m_tags.begin(), m_tags.end(), tag);
  ASSERT(it != m_tags.end());
  if (it != m_tags.end())
    m_tags.erase(it);

  tag->setOwner(nullptr);
}

Tag* Tags::getByName(const std::string& name) const
{
  for (Tag* tag : *this) {
    if (tag->name() == name)
      return tag;
  }
  return nullptr;
}

Tag* Tags::getById(ObjectId id) const
{
  for (Tag* tag : *this) {
    if (tag->id() == id)
      return tag;
  }
  return nullptr;
}

Tag* Tags::innerTag(const frame_t frame) const
{
  const Tag* found = nullptr;
  for (const Tag* tag : *this) {
    if (frame >= tag->fromFrame() &&
        frame <= tag->toFrame()) {
      if (!found ||
          (tag->toFrame() - tag->fromFrame()) < (found->toFrame() - found->fromFrame())) {
        found = tag;
      }
    }
  }
  return const_cast<Tag*>(found);
}

Tag* Tags::outerTag(const frame_t frame) const
{
  const Tag* found = nullptr;
  for (const Tag* tag : *this) {
    if (frame >= tag->fromFrame() &&
        frame <= tag->toFrame()) {
      if (!found ||
          (tag->toFrame() - tag->fromFrame()) > (found->toFrame() - found->fromFrame())) {
        found = tag;
      }
    }
  }
  return const_cast<Tag*>(found);
}

} // namespace doc
