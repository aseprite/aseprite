// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_frame_tag_name.h"

#include "doc/frame_tag.h"

namespace app {
namespace cmd {

SetFrameTagName::SetFrameTagName(FrameTag* tag, const std::string& name)
  : WithFrameTag(tag)
  , m_oldName(tag->name())
  , m_newName(name)
{
}

void SetFrameTagName::onExecute()
{
  frameTag()->setName(m_newName);
  frameTag()->incrementVersion();
}

void SetFrameTagName::onUndo()
{
  frameTag()->setName(m_oldName);
  frameTag()->incrementVersion();
}

} // namespace cmd
} // namespace app
