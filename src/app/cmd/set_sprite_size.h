// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_SET_SPRITE_SIZE_H_INCLUDED
#define APP_CMD_SET_SPRITE_SIZE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"

namespace app {
namespace cmd {
  using namespace doc;

  class SetSpriteSize : public Cmd
                      , public WithSprite {
  public:
    SetSpriteSize(Sprite* sprite, int newWidth, int newHeight);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    uint32_t m_oldWidth;
    uint32_t m_oldHeight;
    uint32_t m_newWidth;
    uint32_t m_newHeight;
  };

} // namespace cmd
} // namespace app

#endif
