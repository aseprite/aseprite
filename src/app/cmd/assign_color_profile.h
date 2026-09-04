// Aseprite
// Copyright (C) 2018-present  Igara Studio S.A.
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
  CMDTYPE('c', 'p', 'C', 's');

  AssignColorProfile(doc::Sprite* sprite, const gfx::ColorSpaceRef& cs);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
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
