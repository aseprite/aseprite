// Aseprite Document Library
// Copyright (C) 2019-2022  Igara Studio S.A.
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

#include <algorithm>

namespace doc {

Tag::Tag(frame_t from, frame_t to)
  : WithUserData(ObjectType::Tag)
  , m_owner(nullptr)
  , m_from(from)
  , m_to(to)
  , m_name("Tag")
{
  color_t defaultColor = rgba_a_mask; // black color with full opacity.
  userData().setColor(defaultColor);
}

Tag::Tag(const Tag& other)
  : WithUserData(ObjectType::Tag)
  , m_owner(nullptr)
  , m_from(other.m_from)
  , m_to(other.m_to)
  , m_name(other.m_name)
  , m_aniDir(other.m_aniDir)
  , m_repeat(other.m_repeat)
{
}

Tag::~Tag()
{
  ASSERT(!m_owner);
}

Sprite* Tag::sprite() const
{
  if (m_owner)
    return m_owner->sprite();
  else
    return nullptr;
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
  userData().setColor(color);
}

void Tag::setAniDir(AniDir aniDir)
{
  ASSERT(m_aniDir == AniDir::FORWARD || m_aniDir == AniDir::REVERSE ||
         m_aniDir == AniDir::PING_PONG || m_aniDir == AniDir::PING_PONG_REVERSE);

  m_aniDir = aniDir;
}

void Tag::setRepeat(int repeat)
{
  m_repeat = std::clamp(repeat, 0, kMaxRepeat);
}

} // namespace doc
