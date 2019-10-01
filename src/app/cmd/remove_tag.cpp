// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remove_tag.h"

namespace app {
namespace cmd {

using namespace doc;

RemoveTag::RemoveTag(Sprite* sprite, Tag* tag)
  : AddTag(sprite, tag)
{
}

void RemoveTag::onExecute()
{
  AddTag::onUndo();
}

void RemoveTag::onUndo()
{
  AddTag::onRedo();
}

void RemoveTag::onRedo()
{
  AddTag::onUndo();
}

} // namespace cmd
} // namespace app
