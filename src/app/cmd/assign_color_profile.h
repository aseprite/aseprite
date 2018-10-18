// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ASSIGN_COLOR_PROFILE_H_INCLUDED
#define APP_CMD_ASSIGN_COLOR_PROFILE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "gfx/color_space.h"

namespace app {
namespace cmd {

  class AssignColorProfile : public Cmd,
                             public WithSprite {
  public:
    AssignColorProfile(doc::Sprite* sprite, const gfx::ColorSpacePtr& cs);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        2*sizeof(gfx::ColorSpace) +
        m_oldCS->iccSize() +
        m_newCS->iccSize();
    }

  private:
    gfx::ColorSpacePtr m_oldCS;
    gfx::ColorSpacePtr m_newCS;
  };

} // namespace cmd
} // namespace app

#endif
