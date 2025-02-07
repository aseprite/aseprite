// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_COPY_FRAME_H_INCLUDED
#define APP_CMD_COPY_FRAME_H_INCLUDED
#pragma once

#include "app/cmd/with_sprite.h"
#include "app/cmd_sequence.h"
#include "doc/frame.h"

namespace app { namespace cmd {
using namespace doc;

class CopyFrame : public CmdSequence,
                  public WithSprite {
public:
  CopyFrame(Sprite* sprite, frame_t fromFrame, frame_t newFrame);

protected:
  void onExecute() override;
  size_t onMemSize() const override
  {
    return sizeof(*this) + CmdSequence::onMemSize() - sizeof(CmdSequence);
  }

private:
  frame_t m_fromFrame;
  frame_t m_newFrame;
};

}} // namespace app::cmd

#endif
