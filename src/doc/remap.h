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

  class Remap {
  public:
    Remap(int entries) : m_map(entries, 0) { }

    // Creates a map to move a set of selected entries before the
    // given index "beforeIndex".
    static Remap moveSelectedEntriesTo(
      const std::vector<bool>& selectedEntries, int beforeIndex);

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

  private:
    std::vector<int> m_map;
  };

} // namespace doc

#endif
