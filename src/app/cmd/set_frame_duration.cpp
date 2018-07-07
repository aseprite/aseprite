// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_frame_duration.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetFrameDuration::SetFrameDuration(Sprite* sprite, frame_t frame, int duration)
  : WithSprite(sprite)
  , m_frame(frame)
  , m_oldDuration(sprite->frameDuration(frame))
  , m_newDuration(duration)
{
}

void SetFrameDuration::onExecute()
{
  sprite()->setFrameDuration(m_frame, m_newDuration);
  sprite()->incrementVersion();
}

void SetFrameDuration::onUndo()
{
  sprite()->setFrameDuration(m_frame, m_oldDuration);
  sprite()->incrementVersion();
}

void SetFrameDuration::onFireNotifications()
{
  Sprite* sprite = this->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(m_frame);
  doc->notify_observers<DocEvent&>(&DocObserver::onFrameDurationChanged, ev);
}

} // namespace cmd
} // namespace app
