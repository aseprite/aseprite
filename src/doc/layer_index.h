// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_INDEX_H_INCLUDED
#define DOC_LAYER_INDEX_H_INCLUDED
#pragma once

namespace doc {

  class LayerIndex {
  public:
    static const LayerIndex NoLayer;

    LayerIndex() : m_value(0) { }
    explicit LayerIndex(int value) : m_value(value) { }

    LayerIndex next(int i = 1) const { return LayerIndex(m_value+i); };
    LayerIndex previous(int i = 1) const { return LayerIndex(m_value-i); };

    operator int() { return m_value; }
    operator const int() const { return m_value; }

    LayerIndex& operator=(const LayerIndex& o) { m_value = o.m_value; return *this; }
    LayerIndex& operator++() { ++m_value; return *this; }
    LayerIndex& operator--() { --m_value; return *this; }
    LayerIndex operator++(int) { LayerIndex old(*this); ++m_value; return old; }
    LayerIndex operator--(int) { LayerIndex old(*this); --m_value; return old; }
    LayerIndex& operator+=(const LayerIndex& o) { m_value += o.m_value; return *this; }
    LayerIndex& operator-=(const LayerIndex& o) { m_value -= o.m_value; return *this; }
    bool operator<(const LayerIndex& o) const { return m_value < o.m_value; }
    bool operator>(const LayerIndex& o) const { return m_value > o.m_value; }
    bool operator<=(const LayerIndex& o) const { return m_value <= o.m_value; }
    bool operator>=(const LayerIndex& o) const { return m_value >= o.m_value; }
    bool operator==(const LayerIndex& o) const { return m_value == o.m_value; }
    bool operator!=(const LayerIndex& o) const { return m_value != o.m_value; }

  private:
    int m_value;
  };

  inline LayerIndex operator+(const LayerIndex& x, const LayerIndex& y) {
    return LayerIndex((int)x + (int)y);
  }

  inline LayerIndex operator-(const LayerIndex& x, const LayerIndex& y) {
    return LayerIndex((int)x - (int)y);
  }

} // namespace doc

#endif
