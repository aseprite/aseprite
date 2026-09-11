// Aseprite
// Copyright (C) 2019-2025  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ADD_SLICE_H_INCLUDED
#define APP_CMD_ADD_SLICE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_slice.h"
#include "app/cmd/with_sprite.h"
#include "app/cmd/with_suspended.h"
#include "doc/slice.h"

namespace app { namespace cmd {
using namespace doc;

class AddSlice : public Cmd,
                 public WithSprite,
                 public WithSlice {
public:
  AddSlice(Sprite* sprite, Slice* slice);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_suspendedSlice.size(); }

private:
  void addSlice(Sprite* sprite, Slice* slice);
  void removeSlice(Sprite* sprite, Slice* slice);

  WithSuspended<doc::Slice*> m_suspendedSlice;
};

}} // namespace app::cmd

#endif
