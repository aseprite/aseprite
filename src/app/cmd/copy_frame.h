// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_COPY_FRAME_H_INCLUDED
#define APP_CMD_COPY_FRAME_H_INCLUDED
#pragma once

#include "app/cmd_sequence.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

namespace app {
namespace cmd {
  using namespace doc;

  class CopyFrame : public CmdSequence
                  , public WithSprite {
  public:
    CopyFrame(Sprite* sprite, frame_t fromFrame, frame_t newFrame);

  protected:
    void onExecute() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        CmdSequence::onMemSize() - sizeof(CmdSequence);
    }

  private:
    frame_t m_fromFrame;
    frame_t m_newFrame;
  };

} // namespace cmd
} // namespace app

#endif
