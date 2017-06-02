// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_REMAP_H_INCLUDED
#define DOC_REMAP_H_INCLUDED
#pragma once

#include "base/debug.h"
#include <vector>

namespace doc {

  class Palette;
  class PalettePicks;

  class Remap {
  public:
    Remap(int entries = 1) : m_map(entries, 0) { }

    int size() const {
      return (int)m_map.size();
    }

    // Maps input "fromIndex" value, to "toIndex" output.
    void map(int fromIndex, int toIndex) {
      ASSERT(fromIndex >= 0 && fromIndex < size());
      ASSERT(toIndex >= 0 && toIndex < size());

      m_map[fromIndex] = toIndex;
    }

    int operator[](int index) const {
      //ASSERT(index >= 0 && index < size());
      if (index >= 0 && index < size())
        return m_map[index];
      else
        return index;           // No remap
    }

    void merge(const Remap& other);
    Remap invert() const;

    int getMemSize() const {
      return sizeof(*this) + sizeof(int)*size();
    }

    // Returns true if the remap can be safely used in 8-bit images.
    bool isFor8bit() const;

    // Returns true if the remap can be inverted. Remaps can be
    // inverted when each input is mapped to one output (e.g. two
    // inputs are not mapped to the same output). This kind of remaps
    // are really easy to undone: You can store the inverted remap as
    // undo data, without saving all images' pixels.
    bool isInvertible(const PalettePicks& usedEntries) const;

  private:
    std::vector<int> m_map;
  };

  // Creates a map to move a set of selected entries before the given
  // index "beforeIndex".
  Remap create_remap_to_move_picks(const PalettePicks& picks, int beforeIndex);

  Remap create_remap_to_expand_palette(int size, int count, int beforeIndex);

  Remap create_remap_to_change_palette(
    const Palette* oldPalette, const Palette* newPalette,
    const int oldMaskIndex,
    const bool remapMaskIndex);

} // namespace doc

#endif
