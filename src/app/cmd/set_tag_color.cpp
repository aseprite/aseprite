// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tag_color.h"

#include "doc/tag.h"

namespace app {
namespace cmd {

SetTagColor::SetTagColor(Tag* tag, doc::color_t color)
  : WithTag(tag)
  , m_oldColor(tag->color())
  , m_newColor(color)
{
}

void SetTagColor::onExecute()
{
  tag()->setColor(m_newColor);
  tag()->incrementVersion();
}

void SetTagColor::onUndo()
{
  tag()->setColor(m_oldColor);
  tag()->incrementVersion();
}

} // namespace cmd
} // namespace app
