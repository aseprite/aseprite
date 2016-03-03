// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_RGBMAP_H_INCLUDED
#define DOC_RGBMAP_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "base/disable_copying.h"
#include "doc/object.h"

#include <vector>

namespace doc {

  class Palette;

  // It acts like a cache for Palette:findBestfit() calls.
  class RgbMap : public Object {
    // Bit activated on m_map entries that aren't yet calculated.
    const int INVALID = 256;

  public:
    RgbMap();

    bool match(const Palette* palette) const;
    void regenerate(const Palette* palette, int mask_index);

    int mapColor(int r, int g, int b, int a) const {
      ASSERT(r >= 0 && r < 256);
      ASSERT(g >= 0 && g < 256);
      ASSERT(b >= 0 && b < 256);
      ASSERT(a >= 0 && a < 256);
      // bits -> bbbbbgggggrrrrraaa
      int i = (a>>5) | ((b>>3) << 3) | ((g>>3) << 8) | ((r>>3) << 13);
      int v = m_map[i];
      return (v & INVALID) ? generateEntry(i, r, g, b, a): v;
    }

    int maskIndex() const { return m_maskIndex; }

  private:
    int generateEntry(int i, int r, int g, int b, int a) const;

    mutable std::vector<uint16_t> m_map;
    const Palette* m_palette;
    int m_modifications;
    int m_maskIndex;

    DISABLE_COPYING(RgbMap);
  };

} // namespace doc

#endif
