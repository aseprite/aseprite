// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/loop_tag.h"

#include "doc/sprite.h"
#include "doc/frame_tag.h"

namespace app {

const char* kLoopTagName = "Loop";

doc::FrameTag* get_animation_tag(const doc::Sprite* sprite, doc::frame_t frame)
{
  return sprite->frameTags().innerTag(frame);
}

doc::FrameTag* get_loop_tag(const doc::Sprite* sprite)
{
  // Get tag with special "Loop" name
  for (doc::FrameTag* tag : sprite->frameTags())
    if (tag->name() == kLoopTagName)
      return tag;

  return nullptr;
}

doc::FrameTag* create_loop_tag(doc::frame_t from, doc::frame_t to)
{
  doc::FrameTag* tag = new doc::FrameTag(from, to);
  tag->setName(kLoopTagName);
  return tag;
}

} // app
