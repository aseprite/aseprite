// Aseprite Document Library
// Copyright (c) 2020-2022 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_RGBMAP_RGB5A3_H_INCLUDED
#define DOC_RGBMAP_RGB5A3_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "base/disable_copying.h"
#include "doc/object.h"
#include "doc/rgbmap.h"

#include <vector>

namespace doc {

  class Palette;

  // It acts like a cache for Palette:findBestfit() calls.
  class RgbMapRGB5A3 : public RgbMap {
    // Bit activated on m_map entries that aren't yet calculated.
    const int INVALID = 256;

  public:
    RgbMapRGB5A3();

    // RgbMap impl
    void regenerateMap(const Palette* palette, int maskIndex) override;
    int mapColor(const color_t rgba) const override {
      const int r = rgba_getr(rgba);
      const int g = rgba_getg(rgba);
      const int b = rgba_getb(rgba);
      const int a = rgba_geta(rgba);
      // bits -> bbbbbgggggrrrrraaa
      const int i = (a>>5) | ((b>>3) << 3) | ((g>>3) << 8) | ((r>>3) << 13);
      const int v = m_map[i];
      return (v & INVALID) ? generateEntry(i, r, g, b, a): v;
    }

    int maskIndex() const override { return m_maskIndex; }

  private:
    int generateEntry(int i, int r, int g, int b, int a) const;

    mutable std::vector<uint16_t> m_map;
    const Palette* m_palette;
    int m_modifications;
    int m_maskIndex;

    DISABLE_COPYING(RgbMapRGB5A3);
  };

} // namespace doc

#endif
