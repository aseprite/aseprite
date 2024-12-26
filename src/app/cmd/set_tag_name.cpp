// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_tag_name.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "doc/tags.h"

namespace app { namespace cmd {

SetTagName::SetTagName(doc::Tag* tag, const std::string& name)
  : WithTag(tag)
  , m_oldName(tag->name())
  , m_newName(name)
{
}

void SetTagName::onExecute()
{
  tag()->setName(m_newName);
  tag()->incrementVersion();
}

void SetTagName::onUndo()
{
  tag()->setName(m_oldName);
  tag()->incrementVersion();
}

void SetTagName::onFireNotifications()
{
  Tag* tag = this->tag();
  Sprite* sprite = tag->owner()->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.tag(tag);
  doc->notify_observers<DocEvent&>(&DocObserver::onTagRename, ev);
}

}} // namespace app::cmd
