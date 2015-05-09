// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_REMAP_H_INCLUDED
#define DOC_REMAP_H_INCLUDED
#pragma once

#include <vector>

namespace doc {

  class PalettePicks;

  class Remap {
  public:
    Remap(int entries) : m_map(entries, 0) { }

    int size() const {
      return (int)m_map.size();
    }

    void map(int fromIndex, int toIndex) {
      ASSERT(fromIndex >= 0 && fromIndex < size());
      ASSERT(toIndex >= 0 && toIndex < size());

      m_map[fromIndex] = toIndex;
    }

    int operator[](int index) const {
      ASSERT(index >= 0 && index < size());
      return m_map[index];
    }

    void merge(const Remap& other);
    Remap invert() const;

    int getMemSize() const {
      return sizeof(*this) + sizeof(int)*size();
    }

  private:
    std::vector<int> m_map;
  };

  // Creates a map to move a set of selected entries before the given
  // index "beforeIndex".
  Remap create_remap_to_move_picks(const PalettePicks& picks, int beforeIndex);

  Remap create_remap_to_expand_palette(int size, int count, int beforeIndex);

} // namespace doc

#endif
