// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMOVE_TAG_H_INCLUDED
#define APP_CMD_REMOVE_TAG_H_INCLUDED
#pragma once

#include "app/cmd/add_tag.h"

namespace app { namespace cmd {
using namespace doc;

class RemoveTag : public AddTag {
public:
  RemoveTag(Sprite* sprite, Tag* tag);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onRedo(Context* ctx) override;
};

}} // namespace app::cmd

#endif
