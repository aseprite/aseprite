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

#include "app/cmd/remove_frame.h"

#include "app/cmd/remove_cel.h"
#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

RemoveFrame::RemoveFrame(Sprite* sprite, frame_t frame)
  : WithSprite(sprite)
  , m_frame(frame)
  , m_firstTime(true)
{
  for (Cel* cel : sprite->cels(m_frame))
    m_seq.add(new cmd::RemoveCel(cel));
}

void RemoveFrame::onExecute()
{
  Sprite* sprite = this->sprite();
  Document* doc = sprite->document();

  if (m_firstTime) {
    m_firstTime = false;
    m_seq.execute(context());
  }
  else
    m_seq.redo();

  sprite->removeFrame(m_frame);

  // Notify observers.
  DocumentEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(m_frame);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onRemoveFrame, ev);
}

void RemoveFrame::onUndo()
{
  Sprite* sprite = this->sprite();
  Document* doc = sprite->document();

  sprite->addFrame(m_frame);
  m_seq.undo();

  // Notify observers about the new frame.
  DocumentEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(m_frame);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddFrame, ev);
}

} // namespace cmd
} // namespace app
