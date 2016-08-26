// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
  , WithFrameTag(frameTag)
  , m_size(0)
{
}

void AddFrameTag::onExecute()
{
  Sprite* sprite = this->sprite();
  FrameTag* frameTag = this->frameTag();

  sprite->frameTags().add(frameTag);
  sprite->incrementVersion();
}

void AddFrameTag::onUndo()
{
  Sprite* sprite = this->sprite();
  FrameTag* frameTag = this->frameTag();
  write_frame_tag(m_stream, frameTag);
  m_size = size_t(m_stream.tellp());

  sprite->frameTags().remove(frameTag);
  sprite->incrementVersion();
  delete frameTag;
}

void AddFrameTag::onRedo()
{
  Sprite* sprite = this->sprite();
  FrameTag* frameTag = read_frame_tag(m_stream);

  sprite->frameTags().add(frameTag);
  sprite->incrementVersion();

  m_stream.str(std::string());
  m_stream.clear();
  m_size = 0;
}

} // namespace cmd
} // namespace app
