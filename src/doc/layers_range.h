// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYERS_RANGE_H_INCLUDED
#define DOC_LAYERS_RANGE_H_INCLUDED
#pragma once

#include "doc/layer_index.h"
#include "doc/object_id.h"

namespace doc {
  class Layer;
  class Sprite;

  class LayersRange {
  public:
    LayersRange(const Sprite* sprite, LayerIndex first, LayerIndex last);

    class iterator {
    public:
      iterator();
      iterator(const Sprite* sprite, LayerIndex first, LayerIndex last);

      bool operator==(const iterator& other) const {
        return m_layer == other.m_layer;
      }

      bool operator!=(const iterator& other) const {
        return !operator==(other);
      }

      Layer* operator*() const {
        return m_layer;
      }

      iterator& operator++();

    private:
      Layer* m_layer;
      LayerIndex m_cur, m_last;
    };

    iterator begin() { return m_begin; }
    iterator end() { return m_end; }

  private:
    iterator m_begin, m_end;
  };

} // namespace doc

#endif
