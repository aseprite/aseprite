// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMOVE_FRAME_H_INCLUDED
#define APP_CMD_REMOVE_FRAME_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "app/cmd_sequence.h"
#include "doc/frame.h"

namespace app {
namespace cmd {
  using namespace doc;

  class RemoveFrame : public Cmd
                    , public WithSprite {
  public:
    RemoveFrame(Sprite* sprite, frame_t frame);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_seq.memSize();
    }

  private:
    frame_t m_frame;
    int m_frameDuration;
    CmdSequence m_seq;
    bool m_firstTime;
    bool m_frameRemoved;
  };

} // namespace cmd
} // namespace app

#endif
