// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/frame_tag.h"

namespace doc {

FrameTag::FrameTag(frame_t from, frame_t to)
  : Object(ObjectType::FrameTag)
  , m_from(from)
  , m_to(to)
  , m_name("Tag")
  , m_color(rgba(0, 0, 0, 255))
{
}

void FrameTag::setFrameRange(frame_t from, frame_t to)
{
  m_from = from;
  m_to = to;
}

void FrameTag::setName(const std::string& name)
{
  m_name = name;
}

void FrameTag::setColor(color_t color)
{
  m_color = color;
}

} // namespace doc
