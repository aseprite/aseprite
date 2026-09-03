// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
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

namespace app { namespace cmd {

class SetGridBounds : public Cmd,
                      public WithSprite {
public:
  CMDTYPE('b', 'o', 'G', 'r');

  SetGridBounds(doc::Sprite* sprite, const gfx::Rect& bounds);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  void setGrid(const gfx::Rect& grid);

  gfx::Rect m_oldBounds;
  gfx::Rect m_newBounds;
};

}} // namespace app::cmd

#endif
