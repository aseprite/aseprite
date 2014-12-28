/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/handle_anidir.h"

#include "app/settings/document_settings.h"
#include "doc/sprite.h"

namespace app {

doc::frame_t calculate_next_frame(
  doc::Sprite* sprite,
  doc::frame_t frame,
  IDocumentSettings* docSettings,
  bool& pingPongForward)
{
  frame_t first = frame_t(0);
  frame_t last = sprite->lastFrame();

  if (docSettings->getLoopAnimation()) {
    frame_t loopBegin, loopEnd;
    docSettings->getLoopRange(&loopBegin, &loopEnd);
    loopBegin = MID(first, loopBegin, last);
    loopEnd = MID(first, loopEnd, last);

    first = loopBegin;
    last = loopEnd;
  }

  switch (docSettings->getAnimationDirection()) {

    case IDocumentSettings::AniDir_Normal:
      ++frame;
      if (frame > last)
        frame = first;
      break;

    case IDocumentSettings::AniDir_Reverse:
      --frame;
      if (frame < first)
        frame = last;
      break;

    case IDocumentSettings::AniDir_PingPong:
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
