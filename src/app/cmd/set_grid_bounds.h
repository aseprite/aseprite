// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_GRID_BOUNDS_H_INCLUDED
#define APP_CMD_SET_GRID_BOUNDS_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "gfx/rect.h"

namespace doc {
  class Sprite;
}

namespace app {
namespace cmd {

  class SetGridBounds : public Cmd
                      , public WithSprite {
  public:
    SetGridBounds(doc::Sprite* sprite, const gfx::Rect& bounds);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    gfx::Rect m_oldBounds;
    gfx::Rect m_newBounds;
  };

} // namespace cmd
} // namespace app

#endif
