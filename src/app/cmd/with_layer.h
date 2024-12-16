// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_WITH_LAYER_H_INCLUDED
#define APP_CMD_WITH_LAYER_H_INCLUDED
#pragma once

#include "doc/object_id.h"

namespace doc {
class Layer;
}

namespace app { namespace cmd {
using namespace doc;

class WithLayer {
public:
  WithLayer(Layer* layer);
  Layer* layer();

private:
  ObjectId m_layerId;
};

}} // namespace app::cmd

#endif
