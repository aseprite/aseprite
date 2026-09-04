// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remove_tag.h"

namespace app { namespace cmd {

using namespace doc;

RemoveTag::RemoveTag(Sprite* sprite, Tag* tag) : AddTag(sprite, tag)
{
}

void RemoveTag::onExecute(Context* ctx)
{
  AddTag::onUndo(ctx);
}

void RemoveTag::onUndo(Context* ctx)
{
  AddTag::onRedo(ctx);
}

void RemoveTag::onRedo(Context* ctx)
{
  AddTag::onUndo(ctx);
}

}} // namespace app::cmd
