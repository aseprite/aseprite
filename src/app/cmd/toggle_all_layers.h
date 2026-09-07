// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_TOGGLE_LAYERS_H_INCLUDED
#define APP_CMD_TOGGLE_LAYERS_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include <vector>

namespace doc {
class Layer;
}

namespace app { namespace cmd {

class ToggleAllLayers : public Cmd,
                        public WithSprite {
public:
  ToggleAllLayers(doc::Sprite* sprite, bool visible);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override;

private:
  void saveVisibilitySnapshot(doc::Layer* layer);
  void restoreVisibilitySnapshot(doc::Layer* layer, size_t& index);
  void applyToggle(doc::Layer* layer);

  bool m_newState;
  std::vector<bool> m_snapshot;
};

}} // namespace app::cmd

#endif
