// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_CEL_FRAME_H_INCLUDED
#define APP_CMD_SET_CEL_FRAME_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "doc/frame.h"

namespace app { namespace cmd {
using namespace doc;

class SetCelFrame : public Cmd,
                    public WithCel {
public:
  CMDTYPE('s', 'f', 'C', 'l');

  SetCelFrame(Cel* cel, frame_t frame);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  frame_t m_oldFrame;
  frame_t m_newFrame;
};

}} // namespace app::cmd

#endif
