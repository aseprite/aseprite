// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_MOVE_CEL_H_INCLUDED
#define APP_CMD_MOVE_CEL_H_INCLUDED
#pragma once

#include "app/cmd/with_layer.h"
#include "app/cmd_sequence.h"
#include "doc/color.h"
#include "doc/frame.h"

namespace app { namespace cmd {

class MoveCel : public CmdSequence {
public:
  MoveCel(doc::Layer* srcLayer,
          doc::frame_t srcFrame,
          doc::Layer* dstLayer,
          doc::frame_t dstFrame,
          bool continuous);

protected:
  void onExecute() override;
  void onFireNotifications() override;

private:
  WithLayer m_srcLayer, m_dstLayer;
  doc::frame_t m_srcFrame, m_dstFrame;
  bool m_continuous;
};

}} // namespace app::cmd

#endif
