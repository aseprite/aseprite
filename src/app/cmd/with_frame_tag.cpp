// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/with_frame_tag.h"

#include "doc/frame_tag.h"

namespace app {
namespace cmd {

using namespace doc;

WithFrameTag::WithFrameTag(FrameTag* frameTag)
  : m_frameTagId(frameTag->id())
{
}

FrameTag* WithFrameTag::frameTag()
{
  return get<FrameTag>(m_frameTagId);
}

} // namespace cmd
} // namespace app
