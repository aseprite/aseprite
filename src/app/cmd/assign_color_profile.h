// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ASSIGN_COLOR_PROFILE_H_INCLUDED
#define APP_CMD_ASSIGN_COLOR_PROFILE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "gfx/color_space.h"

namespace app { namespace cmd {

class AssignColorProfile : public Cmd,
                           public WithSprite {
public:
  AssignColorProfile(doc::Sprite* sprite, const gfx::ColorSpaceRef& cs);

protected:
  void onExecute() override;
  void onUndo() override;
  void onFireNotifications() override;
  size_t onMemSize() const override
  {
    return sizeof(*this) + 2 * sizeof(gfx::ColorSpace) + m_oldCS->iccSize() + m_newCS->iccSize();
  }

private:
  gfx::ColorSpaceRef m_oldCS;
  gfx::ColorSpaceRef m_newCS;
};

}} // namespace app::cmd

#endif
