// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/handle_anidir.h"

#include "doc/frame.h"
#include "doc/frame_tag.h"
#include "doc/sprite.h"

namespace app {

doc::frame_t calculate_next_frame(
  doc::Sprite* sprite,
  doc::frame_t frame,
  doc::FrameTag* tag,
  bool& pingPongForward)
{
  doc::frame_t first = doc::frame_t(0);
  doc::frame_t last = sprite->lastFrame();
  doc::AniDir aniDir = doc::AniDir::FORWARD;

  if (tag) {
    doc::frame_t loopFrom, loopTo;

    loopFrom = tag->fromFrame();
    loopTo   = tag->toFrame();
    loopFrom = MID(first, loopFrom, last);
    loopTo   = MID(first, loopTo, last);

    first  = loopFrom;
    last   = loopTo;
    aniDir = tag->aniDir();
  }

  switch (aniDir) {

    case doc::AniDir::FORWARD:
      ++frame;
      if (frame > last)
        frame = first;
      break;

    case doc::AniDir::REVERSE:
      --frame;
      if (frame < first)
        frame = last;
      break;

    case doc::AniDir::PING_PONG:
      if (pingPongForward) {
        ++frame;
        if (frame > last) {
          frame = last-1;
          if (frame < first)
            frame = first;
          pingPongForward = false;
        }
      }
      else {
        --frame;
        if (frame < first) {
          frame = first+1;
          if (frame > last)
            frame = last;
          pingPongForward = true;
        }
      }
      break;
  }

  return frame;
}

} // namespace app
