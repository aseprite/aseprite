// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_LAYER_FROM_BACKGROUND_H_INCLUDED
#define APP_CMD_LAYER_FROM_BACKGROUND_H_INCLUDED
#pragma once

#include "app/cmd/sequence.h"

namespace doc {
class Layer;
}

namespace app { namespace cmd {
using namespace doc;

class LayerFromBackground : public CmdSequence {
public:
  CMDTYPE('f', 'r', 'B', 'g');

  LayerFromBackground(Layer* layer);
};

}} // namespace app::cmd

#endif
