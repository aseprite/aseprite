// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TRANSPARENT_COLOR_H_INCLUDED
#define APP_CMD_SET_TRANSPARENT_COLOR_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/color.h"

namespace app { namespace cmd {
using namespace doc;

class SetTransparentColor : public Cmd,
                            public WithSprite {
public:
  SetTransparentColor(Sprite* sprite, color_t newMask);

protected:
  void onExecute() override;
  void onUndo() override;
  void onFireNotifications() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  color_t m_oldMaskColor;
  color_t m_newMaskColor;
};

}} // namespace app::cmd

#endif
