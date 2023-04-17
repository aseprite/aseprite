// Aseprite Document Library
// Copyright (c) 2023  Igara Studio S.A.
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

void RenderPlan::addLayer(const Layer* layer,
                          const frame_t frame)
{
  // We cannot add new layers after using processZIndexes()/modified
  // m_items array using z-indexes.
  ASSERT(m_processZIndex == true);

  ++m_order;

  // We can't read this layer
  if (!layer->isVisible())
    return;

  switch (layer->type()) {

    case ObjectType::LayerImage:
    case ObjectType::LayerTilemap: {
      if (Cel* cel = layer->cel(frame)) {
        m_items.emplace_back(m_order, cel);
      }
      break;
    }

    case ObjectType::LayerGroup: {
      for (const auto child : static_cast<const LayerGroup*>(layer)->layers()) {
        addLayer(child, frame);
      }
      break;
    }

  }
}

void RenderPlan::processZIndexes() const
{
  m_processZIndex = false;

  // If all cels has a z-index = 0, we can just use the m_items as it is
  bool noZIndex = true;
  for (int i=0; i<int(m_items.size()); ++i) {
    const int z = m_items[i].cel->zIndex();
    if (z != 0) {
      noZIndex = false;
      break;
    }
  }
  if (noZIndex)
    return;

  // Order cels using its "order" number in m_items array + cel z-index offset
  for (Item& item : m_items)
    item.order = item.order + item.cel->zIndex();
  std::sort(m_items.begin(), m_items.end(),
            [](const Item& a, const Item& b){
              return
                (a.order < b.order) ||
                (a.order == b.order && (a.cel->zIndex() < b.cel->zIndex()));
            });
}

} // namespace doc
