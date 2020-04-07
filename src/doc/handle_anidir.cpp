// Aseprite Document Library
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/handle_anidir.h"

#include "base/clamp.h"
#include "doc/frame.h"
#include "doc/sprite.h"
#include "doc/tag.h"

namespace doc {

frame_t calculate_next_frame(
  const Sprite* sprite,
  frame_t frame,
  frame_t frameDelta,
  const Tag* tag,
  bool& pingPongForward)
{
  if (frameDelta == 0)
    return frame;

  frame_t first = frame_t(0);
  frame_t last = sprite->lastFrame();
  AniDir aniDir = AniDir::FORWARD;

  if (tag) {
    frame_t loopFrom, loopTo;

    loopFrom = tag->fromFrame();
    loopTo   = tag->toFrame();
    loopFrom = base::clamp(loopFrom, first, last);
    loopTo   = base::clamp(loopTo, first, last);

    first  = loopFrom;
    last   = loopTo;
    aniDir = tag->aniDir();
  }

  frame_t frameRange = (last - first + 1);

  switch (aniDir) {

    case AniDir::REVERSE:
      frameDelta = -frameDelta;

    case AniDir::FORWARD:
      frame += frameDelta;
      while (frame > last) frame -= frameRange;
      while (frame < first) frame += frameRange;
      break;

    case AniDir::PING_PONG: {
      bool invertPingPong;
      if (frameDelta < 0) {
        frameDelta = -frameDelta;
        pingPongForward = !pingPongForward;
        invertPingPong = true;
      }
      else
       invertPingPong = false;

      while (--frameDelta >= 0) {
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
      }

      if (invertPingPong)
        pingPongForward = !pingPongForward;
      break;
    }
  }

  return frame;
}

} // namespace doc
