// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/with_tag.h"

#include "doc/tag.h"

namespace app {
namespace cmd {

using namespace doc;

WithTag::WithTag(Tag* tag)
  : m_tagId(tag->id())
{
}

Tag* WithTag::tag()
{
  return get<Tag>(m_tagId);
}

} // namespace cmd
} // namespace app
