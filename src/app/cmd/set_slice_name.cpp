// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_slice_name.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/document.h"
#include "doc/slice.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetSliceName::SetSliceName(Slice* slice, const std::string& name)
  : WithSlice(slice)
  , m_oldName(slice->name())
  , m_newName(name)
{
}

void SetSliceName::onExecute()
{
  Slice* slice = this->slice();
  slice->setName(m_newName);
  slice->incrementVersion();
}

void SetSliceName::onUndo()
{
  Slice* slice = this->slice();
  slice->setName(m_oldName);
  slice->incrementVersion();
}

void SetSliceName::onFireNotifications()
{
  Slice* slice = this->slice();
  Sprite* sprite = slice->owner()->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());
  DocEvent ev(doc);
  ev.sprite(sprite);
  ev.slice(slice);
  doc->notify_observers<DocEvent&>(&DocObserver::onSliceNameChange, ev);
}

} // namespace cmd
} // namespace app
