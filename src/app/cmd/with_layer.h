// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_WITH_LAYER_H_INCLUDED
#define APP_CMD_WITH_LAYER_H_INCLUDED
#pragma once

#include "app/cmd_serial.h"
#include "doc/object_id.h"

namespace doc {
class Layer;
}

namespace app { namespace cmd {
using namespace doc;

class WithLayer {
public:
  WithLayer(Layer* layer = nullptr);
  Layer* layer();

  void serializeLayerId(CmdSerial& s)
  {
    s.serializeObjectId(m_layerId);
    PRINTARGS("serializeLayerId", m_layerId);
  }

private:
  ObjectId m_layerId;
};

}} // namespace app::cmd

#endif
