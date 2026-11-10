// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/toggle_all_layers.h"

#include "doc/layer.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;

ToggleAllLayers::ToggleAllLayers(doc::Sprite* sprite, bool visible)
  : WithSprite(sprite)
  , m_newState(visible)
{
}

void ToggleAllLayers::onExecute()
{
  auto* spr = sprite();
  if (!spr || spr->root()->layersCount() == 0)
    return;

  m_snapshot.clear();

  saveVisibilitySnapshot(spr->root());

  applyToggle(spr->root());
  spr->incrementVersion();
}

void ToggleAllLayers::onUndo()
{
  auto* spr = sprite();
  if (!spr || m_snapshot.empty())
    return;

  size_t index = 0;
  restoreVisibilitySnapshot(spr->root(), index);
  spr->incrementVersion();
}

void ToggleAllLayers::onRedo()
{
  auto* spr = sprite();
  if (!spr)
    return;

  applyToggle(spr->root());
  spr->incrementVersion();
}

size_t ToggleAllLayers::onMemSize() const
{
  return sizeof(*this) + m_snapshot.capacity() * sizeof(bool);
}

void ToggleAllLayers::saveVisibilitySnapshot(Layer* layer)
{
  if (layer != sprite()->root()) {
    m_snapshot.push_back(layer->isVisible());
  }
  if (layer->isGroup()) {
    for (auto* child : static_cast<LayerGroup*>(layer)->layers()) {
      saveVisibilitySnapshot(child);
    }
  }
}

void ToggleAllLayers::restoreVisibilitySnapshot(Layer* layer, size_t& index)
{
  if (layer != sprite()->root()) {
    if (index < m_snapshot.size()) {
      layer->setVisible(m_snapshot[index++]);
      layer->incrementVersion();
    }
  }
  if (layer->isGroup()) {
    for (auto* child : static_cast<LayerGroup*>(layer)->layers()) {
      restoreVisibilitySnapshot(child, index);
    }
  }
}

void ToggleAllLayers::applyToggle(Layer* layer)
{
  if (layer != sprite()->root()) {
    layer->setVisible(m_newState);
    layer->incrementVersion();
  }
  if (layer->isGroup()) {
    for (auto* child : static_cast<LayerGroup*>(layer)->layers()) {
      applyToggle(child);
    }
  }
}

}} // namespace app::cmd
