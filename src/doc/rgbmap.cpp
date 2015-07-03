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
  , m_maskIndex(0)
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
  m_maskIndex = mask_index;

  // Mark all entries as invalid (need to be regenerated)
  for (uint16_t& entry : m_map)
    entry |= INVALID;
}

int RgbMap::generateEntry(int i, int r, int g, int b, int a) const
{
  return m_map[i] =
    m_palette->findBestfit(
      scale_5bits_to_8bits(r>>3),
      scale_5bits_to_8bits(g>>3),
      scale_5bits_to_8bits(b>>3),
      scale_3bits_to_8bits(a>>5), m_maskIndex);
}

} // namespace doc
