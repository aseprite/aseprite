// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_MOVE_LAYER_H_INCLUDED
#define APP_CMD_MOVE_LAYER_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_layer.h"

namespace app { namespace cmd {
using namespace doc;

class MoveLayer : public Cmd {
public:
  CMDTYPE('m', 'v', 'L', 'y');

  MoveLayer(Layer* layer, Layer* newParent, Layer* afterThis);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }
  void onSerialize(CmdSerial& s) override;

private:
  WithLayer m_layer;
  WithLayer m_oldParent, m_oldAfterThis;
  WithLayer m_newParent, m_newAfterThis;
};

}} // namespace app::cmd

#endif
