// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_slice_name.h"

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
  slice()->setName(m_newName);
  slice()->incrementVersion();
}

void SetSliceName::onUndo()
{
  slice()->setName(m_oldName);
  slice()->incrementVersion();
}

} // namespace cmd
} // namespace app
