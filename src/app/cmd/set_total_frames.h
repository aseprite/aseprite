// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_SET_TOTAL_FRAMES_H_INCLUDED
#define APP_CMD_SET_TOTAL_FRAMES_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

namespace app {
namespace cmd {
  using namespace doc;

  class SetTotalFrames : public Cmd
                       , public WithSprite {
  public:
    SetTotalFrames(Sprite* sprite, frame_t frames);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    frame_t m_oldFrames;
    frame_t m_newFrames;
  };

} // namespace cmd
} // namespace app

#endif
