// Aseprite Document Library
// Copyright (c) 2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_RENDER_PLAN_H_INCLUDED
#define DOC_RENDER_PLAN_H_INCLUDED
#pragma once

#include "doc/cel.h"
#include "doc/cel_list.h"
#include "doc/frame.h"

namespace doc {
  class Layer;

  // Creates a list of cels to be rendered in the correct order
  // (depending on layer ordering + z-index) to render the given root
  // layer/layers.
  class RenderPlan {
  public:
    struct Item {
      int order;
      const Layer* layer;
      const Cel* cel;
      int zIndex() const { return (cel ? cel->zIndex(): 0); }
      Item(const int order = 0,
           const Layer* layer = nullptr,
           const Cel* cel = nullptr)
        : order(order), layer(layer), cel(cel) { }
    };
    using Items = std::vector<Item>;

    RenderPlan();

    const Items& items() const {
      if (m_processZIndex)
        processZIndexes();
      return m_items;
    }

    void addLayer(const Layer* layer,
                  const frame_t frame);

  private:
    void processZIndexes() const;

    int m_order = 0;
    mutable Items m_items;
    mutable bool m_processZIndex = true;
  };

} // namespace doc

#endif
