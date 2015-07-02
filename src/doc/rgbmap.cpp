// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
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

#define RSIZE   32
#define GSIZE   32
#define BSIZE   32
#define ASIZE   8
#define MAPSIZE (RSIZE*GSIZE*BSIZE*ASIZE)

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

  // TODO This is slow for 256 colors 32*32*32*8 findBestfit calls

  int i = 0;
  for (int r=0; r<RSIZE; ++r) {
    for (int g=0; g<GSIZE; ++g) {
      for (int b=0; b<BSIZE; ++b) {
        for (int a=0; a<ASIZE; ++a) {
          m_map[i++] =
            palette->findBestfit(
              scale_5bits_to_8bits(r),
              scale_5bits_to_8bits(g),
              scale_5bits_to_8bits(b),
              scale_3bits_to_8bits(a), mask_index);
        }
      }
    }
  }
}

} // namespace doc
