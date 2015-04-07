// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_frame_tag_color.h"

#include "doc/frame_tag.h"

namespace app {
namespace cmd {

SetFrameTagColor::SetFrameTagColor(FrameTag* tag, doc::color_t color)
  : WithFrameTag(tag)
  , m_oldColor(tag->color())
  , m_newColor(color)
{
}

void SetFrameTagColor::onExecute()
{
  frameTag()->setColor(m_newColor);
  frameTag()->incrementVersion();
}

void SetFrameTagColor::onUndo()
{
  frameTag()->setColor(m_oldColor);
  frameTag()->incrementVersion();
}

} // namespace cmd
} // namespace app
