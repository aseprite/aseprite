// Aseprite
// Copyright (C) 2019-2025  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/add_slice.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/slice.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;

AddSlice::AddSlice(Sprite* sprite, Slice* slice) : WithSprite(sprite), WithSlice(slice)
{
}

void AddSlice::onExecute()
{
  Sprite* sprite = this->sprite();
  Slice* slice = this->slice();

  addSlice(sprite, slice);
}

void AddSlice::onUndo()
{
  Sprite* sprite = this->sprite();
  Slice* slice = this->slice();

  removeSlice(sprite, slice);
  m_suspendedSlice.suspend(slice);
}

void AddSlice::onRedo()
{
  Sprite* sprite = this->sprite();
  Slice* slice = m_suspendedSlice.restore();

  addSlice(sprite, slice);
}

void AddSlice::addSlice(Sprite* sprite, Slice* slice)
{
  sprite->slices().add(slice);
  sprite->incrementVersion();

  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.slice(slice);
  doc->notify_observers<DocEvent&>(&DocObserver::onAddSlice, ev);
}

void AddSlice::removeSlice(Sprite* sprite, Slice* slice)
{
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.slice(slice);
  doc->notify_observers<DocEvent&>(&DocObserver::onRemoveSlice, ev);

  sprite->slices().remove(slice);
  sprite->incrementVersion();
}

}} // namespace app::cmd
