// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/restore_visible_layers.h"

#include "doc/layer.h"
#include "doc/selected_layers.h"
#include "doc/sprite.h"

namespace app {

using namespace doc;

RestoreVisibleLayers::~RestoreVisibleLayers()
{
  for (auto item : m_restore)
    item.first->setVisible(item.second);
}

void RestoreVisibleLayers::showLayer(Layer* layer)
{
  SelectedLayers selLayers;
  selLayers.insert(layer);
  selLayers.propagateSelection();
  showSelectedLayers(layer->sprite(), selLayers);
}

void RestoreVisibleLayers::showSelectedLayers(Sprite* sprite, const SelectedLayers& inSelLayers)
{
  SelectedLayers selLayers = inSelLayers;
  selLayers.propagateSelection();
  setLayerVisiblity(sprite->root(), selLayers);
}

void RestoreVisibleLayers::setLayerVisiblity(Layer* group, const SelectedLayers& selLayers)
{
  for (Layer* layer : group->layers()) {
    bool selected = (selLayers.contains(layer));
    if (selected != layer->isVisible()) {
      m_restore.push_back(std::make_pair(layer, layer->isVisible()));
      layer->setVisible(selected);
    }
    if (selected && layer->isGroup())
      setLayerVisiblity(layer, selLayers);
  }
}

} // namespace app
