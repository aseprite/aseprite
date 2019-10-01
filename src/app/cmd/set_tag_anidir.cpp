// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tag_anidir.h"

#include "doc/tag.h"

namespace app {
namespace cmd {

SetTagAniDir::SetTagAniDir(Tag* tag, doc::AniDir anidir)
  : WithTag(tag)
  , m_oldAniDir(tag->aniDir())
  , m_newAniDir(anidir)
{
}

void SetTagAniDir::onExecute()
{
  tag()->setAniDir(m_newAniDir);
  tag()->incrementVersion();
}

void SetTagAniDir::onUndo()
{
  tag()->setAniDir(m_oldAniDir);
  tag()->incrementVersion();
}

} // namespace cmd
} // namespace app
