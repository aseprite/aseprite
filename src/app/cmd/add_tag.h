// Aseprite
// Copyright (C) 2019-2025  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ADD_TAG_H_INCLUDED
#define APP_CMD_ADD_TAG_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "app/cmd/with_suspended.h"
#include "app/cmd/with_tag.h"
#include "doc/tag.h"

namespace app { namespace cmd {
using namespace doc;

class AddTag : public Cmd,
               public WithSprite,
               public WithTag {
public:
  AddTag(Sprite* sprite, Tag* tag);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_suspendedTag.size(); }

private:
  WithSuspended<doc::Tag*> m_suspendedTag;
};

}} // namespace app::cmd

#endif
