// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tag_name.h"

#include "doc/tag.h"

namespace app {
namespace cmd {

SetTagName::SetTagName(doc::Tag* tag, const std::string& name)
  : WithTag(tag)
  , m_oldName(tag->name())
  , m_newName(name)
{
}

void SetTagName::onExecute()
{
  tag()->setName(m_newName);
  tag()->incrementVersion();
}

void SetTagName::onUndo()
{
  tag()->setName(m_oldName);
  tag()->incrementVersion();
}

} // namespace cmd
} // namespace app
