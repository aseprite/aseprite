// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
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

    int mapColor(int r, int g, int b) const;

  private:
    std::vector<uint8_t> m_map;
    const Palette* m_palette;
    int m_modifications;

    DISABLE_COPYING(RgbMap);
  };

} // namespace doc

#endif
