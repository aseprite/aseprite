// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_BACKGROUND_FROM_LAYER_H_INCLUDED
#define APP_CMD_BACKGROUND_FROM_LAYER_H_INCLUDED
#pragma once

#include "app/cmd/with_layer.h"
#include "app/cmd_sequence.h"

namespace app { namespace cmd {
using namespace doc;

class BackgroundFromLayer : public CmdSequence,
                            public WithLayer {
public:
  BackgroundFromLayer(Layer* layer);

protected:
  void onExecute() override;
};

}} // namespace app::cmd

#endif
