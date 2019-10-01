// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/tag.h"

#include "base/debug.h"
#include "doc/tags.h"

namespace doc {

Tag::Tag(frame_t from, frame_t to)
  : Object(ObjectType::Tag)
  , m_owner(nullptr)
  , m_from(from)
  , m_to(to)
  , m_color(rgba(0, 0, 0, 255))
  , m_name("Tag")
  , m_aniDir(AniDir::FORWARD)
{
}

Tag::Tag(const Tag& other)
  : Object(ObjectType::Tag)
  , m_owner(nullptr)
  , m_from(other.m_from)
  , m_to(other.m_to)
  , m_color(other.m_color)
  , m_name(other.m_name)
  , m_aniDir(other.m_aniDir)
{
}

Tag::~Tag()
{
  ASSERT(!m_owner);
}

void Tag::setOwner(Tags* owner)
{
  m_owner = owner;
}

void Tag::setFrameRange(frame_t from, frame_t to)
{
  Tags* owner = m_owner;
  if (owner)
    owner->remove(this);

  m_from = from;
  m_to = to;

  if (owner)
    owner->add(this); // Re-add the tag, so it's added in the correct place
}

void Tag::setName(const std::string& name)
{
  m_name = name;
}

void Tag::setColor(color_t color)
{
  m_color = color;
}

void Tag::setAniDir(AniDir aniDir)
{
  ASSERT(m_aniDir == AniDir::FORWARD ||
         m_aniDir == AniDir::REVERSE ||
         m_aniDir == AniDir::PING_PONG);

  m_aniDir = aniDir;
}

} // namespace doc
