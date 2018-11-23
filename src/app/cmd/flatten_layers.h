// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_FLATTEN_LAYERS_H_INCLUDED
#define APP_CMD_FLATTEN_LAYERS_H_INCLUDED
#pragma once

#include "app/cmd/with_sprite.h"
#include "app/cmd_sequence.h"
#include "doc/object_ids.h"
#include "doc/selected_layers.h"

namespace app {
namespace cmd {

  class FlattenLayers : public CmdSequence
                      , public WithSprite {
  public:
    FlattenLayers(doc::Sprite* sprite,
                  const doc::SelectedLayers& layers);

  protected:
    void onExecute() override;

  private:
    ObjectIds m_layerIds;
  };

} // namespace cmd
} // namespace app

#endif
