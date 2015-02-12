// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_FLATTEN_LAYERS_H_INCLUDED
#define APP_CMD_FLATTEN_LAYERS_H_INCLUDED
#pragma once

#include "app/cmd/with_sprite.h"
#include "app/cmd_sequence.h"

namespace app {
namespace cmd {
  using namespace doc;

  class FlattenLayers : public CmdSequence
                      , public WithSprite {
  public:
    FlattenLayers(Sprite* sprite);

  protected:
    void onExecute() override;
  };

} // namespace cmd
} // namespace app

#endif
