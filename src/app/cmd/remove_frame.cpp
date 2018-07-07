// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remove_frame.h"

#include "app/cmd/remove_cel.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/cels_range.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

RemoveFrame::RemoveFrame(Sprite* sprite, frame_t frame)
  : WithSprite(sprite)
  , m_frame(frame)
  , m_firstTime(true)
  , m_frameRemoved(false)
{
  m_frameDuration = sprite->frameDuration(frame);
  for (Cel* cel : sprite->cels(m_frame))
    m_seq.add(new cmd::RemoveCel(cel));
}

void RemoveFrame::onExecute()
{
  Sprite* sprite = this->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());

  if (m_firstTime) {
    m_firstTime = false;
    m_seq.execute(context());
  }
  else
    m_seq.redo();

  int oldTotalFrames = sprite->totalFrames();

  sprite->removeFrame(m_frame);
  sprite->incrementVersion();

  // True when the number of total frames changed (e.g. this can be
  // false only then we try to delete the last frame and only frame in
  // the sprite).
  m_frameRemoved = (oldTotalFrames != sprite->totalFrames());

  // Notify observers.
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(m_frame);
  doc->notify_observers<DocEvent&>(&DocObserver::onRemoveFrame, ev);
}

void RemoveFrame::onUndo()
{
  Sprite* sprite = this->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());

  if (m_frameRemoved)
    sprite->addFrame(m_frame);
  sprite->setFrameDuration(m_frame, m_frameDuration);
  sprite->incrementVersion();
  m_seq.undo();

  // Notify observers about the new frame.
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(m_frame);
  doc->notify_observers<DocEvent&>(&DocObserver::onAddFrame, ev);
}

} // namespace cmd
} // namespace app
