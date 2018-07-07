// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_total_frames.h"

#include "app/doc.h"
#include "app/doc_event.h"
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
  Sprite* spr = sprite();
  spr->setTotalFrames(m_newFrames);
  spr->incrementVersion();
}

void SetTotalFrames::onUndo()
{
  Sprite* spr = sprite();
  spr->setTotalFrames(m_oldFrames);
  spr->incrementVersion();
}

void SetTotalFrames::onFireNotifications()
{
  Sprite* sprite = this->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(sprite->totalFrames());
  doc->notify_observers<DocEvent&>(&DocObserver::onTotalFramesChanged, ev);
}

} // namespace cmd
} // namespace app
