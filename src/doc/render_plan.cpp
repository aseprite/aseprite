// Aseprite Document Library
// Copyright (c) 2023-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/render_plan.h"

#include "doc/cel.h"
#include "doc/layer.h"

#include <algorithm>
#include <cmath>

namespace doc {

RenderPlan::RenderPlan()
{
}

void RenderPlan::addLayer(const Layer* layer, const frame_t frame)
{
  // We cannot add new layers after using processZIndexes()/modified
  // m_items array using z-indexes.
  ASSERT(m_processZIndex == true);

  ++m_order;
  // Note: Non-visible layers must also be included; they are necessary
  // to process the Z order of the layers, then they will be removed
  // from the vector of items to be rendered.

  if (layer->isGroup() && !m_composeGroups) {
    for (auto* const child : static_cast<const LayerGroup*>(layer)->layers()) {
      addLayer(child, frame);
    }
  }
  else {
    m_items.emplace_back(m_order, layer, layer->cel(frame));
  }
}

void RenderPlan::processZIndexes() const
{
  m_processZIndex = false;

  // If all cels has a z-index = 0, we can skip the sorting step
  bool hasZIndex = false;
  for (int i = 0; i < int(m_items.size()); ++i) {
    const int z = m_items[i].zIndex();
    if (z != 0) {
      hasZIndex = true;
      break;
    }
  }

  if (hasZIndex) {
    // Order cels using its "order" number in m_items array + cel z-index offset
    for (Item& item : m_items)
      item.order = item.order + item.zIndex();
    std::sort(m_items.begin(), m_items.end(), [](const Item& a, const Item& b) {
      return (a.order < b.order) || (a.order == b.order && (a.zIndex() < b.zIndex()));
    });
  }

  // Remove hidden layers. We include all layers in addLayer()
  // to preserve the correct z-order, but we don't need to render
  // non-visible layers.
  Items visibleItems;
  for (const Item& item : m_items) {
    if (item.layer && item.layer->isVisibleHierarchy())
      visibleItems.push_back(item);
  }
  m_items = std::move(visibleItems);
}

} // namespace doc
