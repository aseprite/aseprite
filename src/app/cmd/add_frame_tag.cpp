// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_frame_tag.h"

#include "doc/frame_tag.h"
#include "doc/frame_tag_io.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

AddFrameTag::AddFrameTag(Sprite* sprite, FrameTag* frameTag)
  : WithSprite(sprite)
  , m_frameTag(frameTag)
  , m_frameTagId(0)
{
}

void AddFrameTag::onExecute()
{
  Sprite* sprite = this->sprite();

  if (m_frameTag) {
    m_frameTagId = m_frameTag->id();
  }
  else {
    m_frameTag = read_frame_tag(m_stream);
    m_stream.str(std::string());
    m_stream.clear();
  }

  sprite->frameTags().add(m_frameTag);
  m_frameTag = nullptr;
}

void AddFrameTag::onUndo()
{
  Sprite* sprite = this->sprite();
  FrameTag* frameTag = get<FrameTag>(m_frameTagId);
  write_frame_tag(m_stream, frameTag);

  sprite->frameTags().remove(frameTag);

  delete frameTag;
}

size_t AddFrameTag::onMemSize() const
{
  return sizeof(*this)
    + (m_frameTag ? m_frameTag->getMemSize(): 0)
    + (size_t)const_cast<std::stringstream*>(&m_stream)->tellp();
}

} // namespace cmd
} // namespace app
