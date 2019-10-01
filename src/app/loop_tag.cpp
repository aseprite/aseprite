// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/loop_tag.h"

#include "doc/sprite.h"
#include "doc/tag.h"

namespace app {

const char* kLoopTagName = "Loop";

doc::Tag* get_animation_tag(const doc::Sprite* sprite, doc::frame_t frame)
{
  return sprite->tags().innerTag(frame);
}

doc::Tag* get_loop_tag(const doc::Sprite* sprite)
{
  // Get tag with special "Loop" name
  for (doc::Tag* tag : sprite->tags())
    if (tag->name() == kLoopTagName)
      return tag;

  return nullptr;
}

doc::Tag* create_loop_tag(doc::frame_t from, doc::frame_t to)
{
  doc::Tag* tag = new doc::Tag(from, to);
  tag->setName(kLoopTagName);
  return tag;
}

} // app
