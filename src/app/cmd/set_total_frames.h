// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TOTAL_FRAMES_H_INCLUDED
#define APP_CMD_SET_TOTAL_FRAMES_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

namespace app { namespace cmd {
using namespace doc;

class SetTotalFrames : public Cmd,
                       public WithSprite {
public:
  SetTotalFrames(Sprite* sprite, frame_t frames);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  frame_t m_oldFrames;
  frame_t m_newFrames;
};

}} // namespace app::cmd

#endif
