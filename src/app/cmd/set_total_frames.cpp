/* Aseprite
 * Copyright (C) 2001-2015  David Capello
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

#include "app/cmd/set_total_frames.h"

#include "app/document.h"
#include "doc/document_event.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetTotalFrames::SetTotalFrames(Sprite* sprite, frame_t frames)
  : WithSprite(sprite)
  , m_oldFrames(sprite->totalFrames())
  , m_newFrames(frames)
{
}

void SetTotalFrames::onExecute()
{
  sprite()->setTotalFrames(m_newFrames);
}

void SetTotalFrames::onUndo()
{
  sprite()->setTotalFrames(m_oldFrames);
}

void SetTotalFrames::onFireNotifications()
{
  Sprite* sprite = this->sprite();
  doc::Document* doc = sprite->document();
  DocumentEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(sprite->totalFrames());
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onTotalFramesChanged, ev);
}

} // namespace cmd
} // namespace app
