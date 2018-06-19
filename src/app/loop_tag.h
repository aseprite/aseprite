// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_LOOP_TAG_H_INCLUDED
#define APP_LOOP_TAG_H_INCLUDED
#pragma once

#include "doc/frame.h"

namespace doc {
  class FrameTag;
  class Sprite;
}

namespace app {

  class FrameTagProvider {
  public:
    virtual ~FrameTagProvider() { }
    virtual doc::FrameTag* getFrameTagByFrame(const doc::frame_t frame,
                                              const bool getLoopTagIfNone) = 0;
  };

  doc::FrameTag* get_animation_tag(const doc::Sprite* sprite, doc::frame_t frame);
  doc::FrameTag* get_loop_tag(const doc::Sprite* sprite);
  doc::FrameTag* create_loop_tag(doc::frame_t from, doc::frame_t to);

} // namespace app

#endif
