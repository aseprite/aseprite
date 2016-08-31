// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remove_frame_tag.h"

namespace app {
namespace cmd {

using namespace doc;

RemoveFrameTag::RemoveFrameTag(Sprite* sprite, FrameTag* tag)
  : AddFrameTag(sprite, tag)
{
}

void RemoveFrameTag::onExecute()
{
  AddFrameTag::onUndo();
}

void RemoveFrameTag::onUndo()
{
  AddFrameTag::onRedo();
}

void RemoveFrameTag::onRedo()
{
  AddFrameTag::onUndo();
}

} // namespace cmd
} // namespace app
