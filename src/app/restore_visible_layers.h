// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RESTORE_VISIBLE_LAYERS_H_INCLUDED
#define APP_RESTORE_VISIBLE_LAYERS_H_INCLUDED
#pragma once

#include <vector>

namespace doc {
class Layer;
class LayerGroup;
class SelectedLayers;
class Sprite;
} // namespace doc

namespace app {

class RestoreVisibleLayers {
public:
  ~RestoreVisibleLayers();

  void showLayer(doc::Layer* layer);
  void showSelectedLayers(doc::Sprite* sprite, const doc::SelectedLayers& selLayers);

private:
  void setLayerVisiblity(doc::Layer* group, const doc::SelectedLayers& selLayers);

  std::vector<std::pair<doc::Layer*, bool>> m_restore;
};

} // namespace app

#endif
