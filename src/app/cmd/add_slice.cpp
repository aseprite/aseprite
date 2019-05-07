// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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
#include "doc/slice_io.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

AddSlice::AddSlice(Sprite* sprite, Slice* slice)
  : WithSprite(sprite)
  , WithSlice(slice)
  , m_size(0)
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
  write_slice(m_stream, slice);
  m_size = size_t(m_stream.tellp());

  removeSlice(sprite, slice);
}

void AddSlice::onRedo()
{
  Sprite* sprite = this->sprite();
  Slice* slice = read_slice(m_stream);

  addSlice(sprite, slice);

  m_stream.str(std::string());
  m_stream.clear();
  m_size = 0;
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
  delete slice;
}

} // namespace cmd
} // namespace app
