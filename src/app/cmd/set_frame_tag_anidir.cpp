// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_frame_tag_anidir.h"

#include "doc/frame_tag.h"

namespace app {
namespace cmd {

SetFrameTagAniDir::SetFrameTagAniDir(FrameTag* tag, doc::AniDir anidir)
  : WithFrameTag(tag)
  , m_oldAniDir(tag->aniDir())
  , m_newAniDir(anidir)
{
}

void SetFrameTagAniDir::onExecute()
{
  frameTag()->setAniDir(m_newAniDir);
  frameTag()->incrementVersion();
}

void SetFrameTagAniDir::onUndo()
{
  frameTag()->setAniDir(m_oldAniDir);
  frameTag()->incrementVersion();
}

} // namespace cmd
} // namespace app
