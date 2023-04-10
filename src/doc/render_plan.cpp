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
  // m_cels array using z-indexes.
  ASSERT(m_processZIndex == true);

  // We can't read this layer
  if (!layer->isVisible())
    return;

  switch (layer->type()) {

    case ObjectType::LayerImage:
    case ObjectType::LayerTilemap: {
      if (Cel* cel = layer->cel(frame)) {
        m_cels.push_back(cel);
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

  // If all cels has a z-index = 0, we can just use the m_cels as it is
  bool noZIndex = true;
  for (int i=0; i<int(m_cels.size()); ++i) {
    const int z = m_cels[i]->zIndex();
    if (z != 0) {
      noZIndex = false;
      break;
    }
  }
  if (noZIndex)
    return;

  // Order cels using its real index in the m_cels array + its z-index value
  std::vector<std::pair<int, Cel*>> indexedCels;
  const int n = m_cels.size();
  indexedCels.reserve(n);
  for (int i=0; i<n; ++i) {
    Cel* cel = m_cels[i];
    int z = cel->zIndex();
    indexedCels.push_back(std::make_pair(i+z, cel));
  }
  std::sort(indexedCels.begin(), indexedCels.end(),
            [](const auto& a, const auto& b){
              return
                (a.first < b.first) ||
                (a.first == b.first && (a.second->zIndex() < b.second->zIndex()));
            });
  for (int i=0; i<n; ++i) {
    m_cels[i] = indexedCels[i].second;
  }
}

} // namespace doc
