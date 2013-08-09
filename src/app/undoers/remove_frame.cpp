/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#include "app/undoers/remove_frame.h"

#include "app/document.h"
#include "app/document_event.h"
#include "app/document_observer.h"
#include "app/undoers/add_frame.h"
#include "raster/sprite.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace raster;
using namespace undo;

RemoveFrame::RemoveFrame(undo::ObjectsContainer* objects, Document* document, Sprite* sprite, FrameNumber frame)
  : m_documentId(objects->addObject(document))
  , m_spriteId(objects->addObject(sprite))
  , m_frame(frame)
  , m_frameDuration(sprite->getFrameDuration(frame))
{
}

void RemoveFrame::dispose()
{
  delete this;
}

void RemoveFrame::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Document* document = objects->getObjectT<Document>(m_documentId);
  Sprite* sprite = objects->getObjectT<Sprite>(m_spriteId);

  // TODO Merge this code with DocumentApi::addFrame

  redoers->pushUndoer(new AddFrame(objects, document, sprite, m_frame));

  sprite->addFrame(m_frame);
  sprite->setFrameDuration(m_frame, m_frameDuration);

  // Notify observers.
  DocumentEvent ev(document);
  ev.sprite(sprite);
  ev.frame(m_frame);
  document->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddFrame, ev);
}

} // namespace undoers
} // namespace app
