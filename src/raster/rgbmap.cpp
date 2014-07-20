/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/rgbmap.h"

#include "raster/color_scales.h"
#include "raster/palette.h"

namespace raster {

#define MAPSIZE 32*32*32

RgbMap::RgbMap()
  : Object(OBJECT_RGBMAP)
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

} // namespace raster
