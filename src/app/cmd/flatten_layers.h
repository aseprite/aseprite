// Aseprite
// Copyright (C) 2019 Igara Studio S.A.
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

    enum Options {
      NewBlendMethod  = 0x01,
      Inplace         = 0x02,
      MergeDown       = 0x04,
      ExtendCanvas    = 0x08,
    };

    FlattenLayers(doc::Sprite* sprite,
                  const doc::SelectedLayers& layers,
                  const int options);

  protected:
    void onExecute() override;

  private:
    doc::ObjectIds m_layerIds;
    int m_options;
  };

} // namespace cmd
} // namespace app

#endif
