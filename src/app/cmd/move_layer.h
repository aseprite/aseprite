// Aseprite
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
  MoveLayer(Layer* layer, Layer* newParent, Layer* afterThis);

protected:
  void onExecute() override;
  void onUndo() override;
  void onFireNotifications() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  WithLayer m_layer;
  WithLayer m_oldParent, m_oldAfterThis;
  WithLayer m_newParent, m_newAfterThis;
};

}} // namespace app::cmd

#endif
