// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_RGBMAP_H_INCLUDED
#define DOC_RGBMAP_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/object.h"

#include <vector>

namespace doc {

  class Palette;

  class RgbMap : public Object {
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
      return m_map[(a>>5) | ((b>>3) << 3) | ((g>>3) << 8) | ((r>>3) << 13)];
    }

  private:
    std::vector<uint8_t> m_map;
    const Palette* m_palette;
    int m_modifications;

    DISABLE_COPYING(RgbMap);
  };

} // namespace doc

#endif
