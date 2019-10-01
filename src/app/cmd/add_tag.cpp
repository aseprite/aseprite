// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_tag.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "doc/tag_io.h"

namespace app {
namespace cmd {

using namespace doc;

AddTag::AddTag(Sprite* sprite, Tag* tag)
  : WithSprite(sprite)
  , WithTag(tag)
  , m_size(0)
{
}

void AddTag::onExecute()
{
  Sprite* sprite = this->sprite();
  Tag* tag = this->tag();

  sprite->tags().add(tag);
  sprite->incrementVersion();

  // Notify observers about the new frame.
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.tag(tag);
  doc->notify_observers<DocEvent&>(&DocObserver::onAddTag, ev);
}

void AddTag::onUndo()
{
  Sprite* sprite = this->sprite();
  Tag* tag = this->tag();
  write_tag(m_stream, tag);
  m_size = size_t(m_stream.tellp());

  // Notify observers about the new frame.
  {
    Doc* doc = static_cast<Doc*>(sprite->document());
    DocEvent ev(doc);
    ev.sprite(sprite);
    ev.tag(tag);
    doc->notify_observers<DocEvent&>(&DocObserver::onRemoveTag, ev);
  }

  sprite->tags().remove(tag);
  sprite->incrementVersion();
  delete tag;
}

void AddTag::onRedo()
{
  Sprite* sprite = this->sprite();
  Tag* tag = read_tag(m_stream);

  sprite->tags().add(tag);
  sprite->incrementVersion();

  m_stream.str(std::string());
  m_stream.clear();
  m_size = 0;

  // Notify observers about the new frame.
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.tag(tag);
  doc->notify_observers<DocEvent&>(&DocObserver::onAddTag, ev);
}

} // namespace cmd
} // namespace app
