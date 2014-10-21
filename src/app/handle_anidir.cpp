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

doc::FrameNumber calculate_next_frame(
  doc::Sprite* sprite,
  doc::FrameNumber frame,
  IDocumentSettings* docSettings,
  bool& pingPongForward)
{
  FrameNumber first = FrameNumber(0);
  FrameNumber last = sprite->lastFrame();

  if (docSettings->getLoopAnimation()) {
    FrameNumber loopBegin, loopEnd;
    docSettings->getLoopRange(&loopBegin, &loopEnd);
    loopBegin = MID(first, loopBegin, last);
    loopEnd = MID(first, loopEnd, last);

    first = loopBegin;
    last = loopEnd;
  }

  switch (docSettings->getAnimationDirection()) {

    case IDocumentSettings::AniDir_Normal:
      frame = frame.next();
      if (frame > last) frame = first;
      break;

    case IDocumentSettings::AniDir_Reverse:
      frame = frame.previous();
      if (frame < first) frame = last;
      break;

    case IDocumentSettings::AniDir_PingPong:
      if (pingPongForward) {
        frame = frame.next();
        if (frame > last) {
          frame = last.previous();
          if (frame < first) frame = first;
          pingPongForward = false;
        }
      }
      else {
        frame = frame.previous();
        if (frame < first) {
          frame = first.next();
          if (frame > last) frame = last;
          pingPongForward = true;
        }
      }
      break;
  }

  return frame;
}

} // namespace app
