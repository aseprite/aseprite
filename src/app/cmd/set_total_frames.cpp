// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
  doc::Document* doc = sprite->document();
  DocumentEvent ev(doc);
  ev.sprite(sprite);
  ev.frame(sprite->totalFrames());
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onTotalFramesChanged, ev);
}

} // namespace cmd
} // namespace app
