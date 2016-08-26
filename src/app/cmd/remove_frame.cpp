// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remove_frame.h"

#include "app/cmd/remove_cel.h"
#include "doc/cels_range.h"
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
  m_frameDuration = sprite->frameDuration(frame);
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
  sprite->incrementVersion();

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
  sprite->setFrameDuration(m_frame, m_frameDuration);
  sprite->incrementVersion();
  m_seq.undo();

  // Notify observers about the new frame.
  DocumentEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(m_frame);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddFrame, ev);
}

} // namespace cmd
} // namespace app
