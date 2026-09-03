// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
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

namespace app { namespace cmd {

SetFrameDuration::SetFrameDuration(Sprite* sprite, frame_t frame, int duration)
  : WithSprite(sprite)
  , m_frame(frame)
  , m_oldDuration(sprite->frameDuration(frame))
  , m_newDuration(duration)
{
}

void SetFrameDuration::onExecute(Context* ctx)
{
  sprite()->setFrameDuration(m_frame, m_newDuration);
  sprite()->incrementVersion();
}

void SetFrameDuration::onUndo(Context* ctx)
{
  sprite()->setFrameDuration(m_frame, m_oldDuration);
  sprite()->incrementVersion();
}

void SetFrameDuration::onFireNotifications(Context* ctx)
{
  Sprite* sprite = this->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(m_frame);
  doc->notify_observers<DocEvent&>(&DocObserver::onFrameDurationChanged, ev);
}

void SetFrameDuration::onSerialize(CmdSerial& s)
{
  Cmd::onSerialize(s);
  s(m_frame);
  s(m_oldDuration);
  s(m_newDuration);
}

}} // namespace app::cmd
