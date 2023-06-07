// Aseprite Document Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/selected_layers.h"

#include "base/base.h"
#include "base/debug.h"
#include "base/serialization.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void SelectedLayers::clear()
{
  m_set.clear();
}

void SelectedLayers::insert(Layer* layer)
{
  ASSERT(layer);
  m_set.insert(layer);
}

void SelectedLayers::erase(const Layer* layer)
{
  m_set.erase(const_cast<Layer*>(layer));
}

bool SelectedLayers::contains(const Layer* layer) const
{
  return m_set.find(const_cast<Layer*>(layer)) != m_set.end();
}

bool SelectedLayers::hasSameParent() const
{
  Layer* parent = nullptr;
  for (auto layer : *this) {
    if (parent) {
      if (layer->parent() != parent)
        return false;
    }
    else
      parent = layer->parent();
  }
  return true;
}

LayerList SelectedLayers::toAllLayersList() const
{
  LayerList output;

  if (empty())
    return output;

  ASSERT(*begin());
  ASSERT((*begin())->sprite());

  for (Layer* layer = (*begin())->sprite()->firstLayer();
       layer != nullptr;
       layer = layer->getNextInWholeHierarchy()) {
    if (contains(layer))
      output.push_back(layer);
  }

  return output;
}

LayerList SelectedLayers::toBrowsableLayerList() const
{
  LayerList output;

  if (empty())
    return output;

  ASSERT(*begin());
  ASSERT((*begin())->sprite());

  for (Layer* layer = (*begin())->sprite()->firstBrowsableLayer();
       layer != nullptr;
       layer = layer->getNextBrowsable()) {
    if (contains(layer))
      output.push_back(layer);
  }

  return output;
}

LayerList SelectedLayers::toAllTilemaps() const
{
  LayerList output;

  if (empty())
    return output;

  for (Layer* layer : *this) {
    if (layer->isGroup()) {
      auto group = static_cast<LayerGroup*>(layer);
      group->allTilemaps(output);
    } else if (layer->isTilemap())
      output.push_back(layer);
  }

  return output;
}

void SelectedLayers::removeChildrenIfParentIsSelected()
{
  SelectedLayers removeThese;

  for (Layer* child : *this) {
    Layer* parent = child->parent();
    while (parent) {
      if (contains(parent)) {
        removeThese.insert(child);
        break;
      }
      parent = parent->parent();
    }
  }

  for (Layer* child : removeThese)
    erase(child);
}

void SelectedLayers::selectAllLayers(LayerGroup* group)
{
  for (Layer* layer : group->layers()) {
    if (layer->isGroup())
      selectAllLayers(static_cast<LayerGroup*>(layer));
    insert(layer);
  }
}

void SelectedLayers::expandCollapsedGroups()
{
  auto copy = m_set;
  for (Layer* layer : copy) {
    if (layer->isGroup() && layer->isCollapsed())
      selectAllLayers(static_cast<LayerGroup*>(layer));
  }
}

void SelectedLayers::displace(layer_t layerDelta)
{
  // Do nothing case
  if (layerDelta == 0)
    return;

  const SelectedLayers original = *this;

retry:;
  clear();

  for (auto it : original) {
    Layer* layer = it;

    if (layerDelta > 0) {
      for (layer_t i=0; layer && i<layerDelta; ++i)
        layer = layer->getNextBrowsable();
    }
    else if (layerDelta < 0) {
      for (layer_t i=0; layer && i>layerDelta; --i) {
        layer = layer->getPreviousBrowsable();
      }
    }

    if (layer) {
      insert(layer);
    }
    // If some layer is outside the range it means that the delta is
    // too big (out of bounds), we reduce the delta and try again the
    // whole process.
    else {
      layerDelta -= SGN(layerDelta);
      if (layerDelta == 0) {
        m_set = original.m_set;
        break;
      }
      goto retry;
    }
  }
}

// This will select:
// 1. all visible children in case the parent is selected and none of
//    its children is selected.
// 2. all parent if one children is selected
void SelectedLayers::propagateSelection()
{
  SelectedLayers newSel;

  for (Layer* layer : *this) {
    if (!layer->isGroup())
      continue;

    LayerList children;
    static_cast<LayerGroup*>(layer)->allLayers(children);

    bool allDeselected = true;
    for (Layer* child : children) {
      if (contains(child)) {
        allDeselected = false;
        break;
      }
    }
    if (allDeselected) {
      for (Layer* child : children) {
        if (child->isVisible())
          newSel.insert(child);
      }
    }
  }

  for (Layer* layer : *this) {
    Layer* parent = layer->parent();
    while (parent != layer->sprite()->root() &&
           !contains(parent)) {
      newSel.insert(parent);
      parent = parent->parent();
    }
  }

  for (Layer* layer : newSel)
    insert(layer);
}

bool SelectedLayers::write(std::ostream& os) const
{
  write32(os, size());
  for (const Layer* layer : *this)
    write32(os, layer->id());
  return os.good();
}

bool SelectedLayers::read(std::istream& is)
{
  clear();

  int nlayers = read32(is);
  for (int i=0; i<nlayers && is; ++i) {
    ObjectId id = read32(is);
    Layer* layer = doc::get<Layer>(id);

    // Check that the layer does exist. You will see a little trick in
    // UndoCommand::onExecute() deserializing the DocumentRange stream
    // after the undo/redo is executed so layers exist at this point.

    // TODO This should be an assert, but there is a bug that make this fail
    //ASSERT(layer);

    if (layer)
      insert(layer);
  }
  return is.good();
}

} // namespace doc
