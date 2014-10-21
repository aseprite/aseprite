// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/rgbmap.h"

#include "doc/color_scales.h"
#include "doc/palette.h"

namespace doc {

#define MAPSIZE 32*32*32

RgbMap::RgbMap()
  : Object(ObjectType::RgbMap)
  , m_map(MAPSIZE)
  , m_palette(NULL)
  , m_modifications(0)
{
}

bool RgbMap::match(const Palette* palette) const
{
  return (m_palette == palette &&
    m_modifications == palette->getModifications());
}

void RgbMap::regenerate(const Palette* palette, int mask_index)
{
  m_palette = palette;
  m_modifications = palette->getModifications();

  int i = 0;
  for (int r=0; r<32; ++r) {
    for (int g=0; g<32; ++g) {
      for (int b=0; b<32; ++b) {
        m_map[i++] =
          palette->findBestfit(
            scale_5bits_to_8bits(r),
            scale_5bits_to_8bits(g),
            scale_5bits_to_8bits(b), mask_index);
      }
    }
  }
}

int RgbMap::mapColor(int r, int g, int b) const
{
  ASSERT(r >= 0 && r < 256);
  ASSERT(g >= 0 && g < 256);
  ASSERT(b >= 0 && b < 256);
  return m_map[((r>>3) << 10) + ((g>>3) << 5) + (b>>3)];
}

} // namespace doc
