// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_LAYER_TILESET_H_INCLUDED
#define APP_CMD_SET_LAYER_TILESET_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_layer.h"
#include "doc/tile.h"

namespace doc {
  class LayerTilemap;
}

namespace app {
namespace cmd {
  using namespace doc;

  class SetLayerTileset : public Cmd
                        , public WithLayer {
  public:
    SetLayerTileset(doc::LayerTilemap* layer,
                    doc::tileset_index tsi);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    doc::tileset_index m_oldTsi;
    doc::tileset_index m_newTsi;
  };

} // namespace cmd
} // namespace app

#endif
