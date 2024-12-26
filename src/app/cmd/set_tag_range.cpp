// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_tag_range.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "doc/tags.h"

namespace app { namespace cmd {

SetTagRange::SetTagRange(Tag* tag, frame_t from, frame_t to)
  : WithTag(tag)
  , m_oldFrom(tag->fromFrame())
  , m_oldTo(tag->toFrame())
  , m_newFrom(from)
  , m_newTo(to)
{
}

void SetTagRange::onExecute()
{
  tag()->setFrameRange(m_newFrom, m_newTo);
  tag()->incrementVersion();
}

void SetTagRange::onUndo()
{
  tag()->setFrameRange(m_oldFrom, m_oldTo);
  tag()->incrementVersion();
}

void SetTagRange::onFireNotifications()
{
  Tag* tag = this->tag();
  Sprite* sprite = tag->owner()->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.tag(tag);
  doc->notify_observers<DocEvent&>(&DocObserver::onTagChange, ev);
}

}} // namespace app::cmd
