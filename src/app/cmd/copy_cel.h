// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_COPY_CEL_H_INCLUDED
#define APP_CMD_COPY_CEL_H_INCLUDED
#pragma once

#include "app/cmd/with_layer.h"
#include "app/cmd_sequence.h"
#include "doc/color.h"
#include "doc/frame.h"

namespace app { namespace cmd {
using namespace doc;

class CopyCel : public CmdSequence {
public:
  CopyCel(Layer* srcLayer, frame_t srcFrame, Layer* dstLayer, frame_t dstFrame, bool continuous);

protected:
  void onExecute() override;
  void onFireNotifications() override;

private:
  WithLayer m_srcLayer, m_dstLayer;
  frame_t m_srcFrame, m_dstFrame;
  bool m_continuous;
};

}} // namespace app::cmd

#endif
