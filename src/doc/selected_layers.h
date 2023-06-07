// Aseprite Document Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SELECTED_LAYERS_H_INCLUDED
#define DOC_SELECTED_LAYERS_H_INCLUDED
#pragma once

#include "layer_list.h"

#include <iosfwd>
#include <set>

namespace doc {

  class Layer;
  class LayerGroup;

  class SelectedLayers {
  public:
    typedef std::set<Layer*> Set;
    typedef Set::iterator iterator;
    typedef Set::const_iterator const_iterator;

    iterator begin() { return m_set.begin(); }
    iterator end() { return m_set.end(); }
    const_iterator begin() const { return m_set.begin(); }
    const_iterator end() const { return m_set.end(); }

    bool empty() const { return m_set.empty(); }
    layer_t size() const { return (layer_t)m_set.size(); }

    void clear();
    void insert(Layer* layer);
    void erase(const Layer* layer);

    bool contains(const Layer* layer) const;
    bool hasSameParent() const;
    LayerList toBrowsableLayerList() const;
    LayerList toAllLayersList() const;
    LayerList toAllTilemaps() const;

    void removeChildrenIfParentIsSelected();
    void expandCollapsedGroups();
    void selectAllLayers(LayerGroup* group);
    void displace(layer_t layerDelta);

    void propagateSelection();

    bool operator==(const SelectedLayers& o) const {
      return m_set == o.m_set;
    }

    bool operator!=(const SelectedLayers& o) const {
      return !operator==(o);
    }

    bool write(std::ostream& os) const;
    bool read(std::istream& is);

  private:
    Set m_set;
  };

} // namespace doc

#endif  // DOC_SELECTED_LAYERS_H_INCLUDED
