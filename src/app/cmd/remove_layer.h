// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMOVE_LAYER_H_INCLUDED
#define APP_CMD_REMOVE_LAYER_H_INCLUDED
#pragma once

#include "app/cmd/add_layer.h"

namespace app { namespace cmd {
using namespace doc;

class RemoveLayer : public AddLayer {
public:
  RemoveLayer(Layer* layer);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
};

}} // namespace app::cmd

#endif
