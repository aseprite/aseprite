// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_PIXEL_RATIO_H_INCLUDED
#define APP_CMD_SET_PIXEL_RATIO_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/pixel_ratio.h"

namespace doc {
  class Sprite;
}

namespace app {
namespace cmd {
  using namespace doc;

  class SetPixelRatio : public Cmd
                      , public WithSprite {
  public:
    SetPixelRatio(Sprite* sprite, PixelRatio pixelRatio);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    PixelRatio m_oldRatio;
    PixelRatio m_newRatio;
  };

} // namespace cmd
} // namespace app

#endif
