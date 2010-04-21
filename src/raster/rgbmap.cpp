/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "config.h"

#include <cassert>
#include <allegro.h>

#include "raster/palette.h"
#include "raster/rgbmap.h"

class RgbMapImpl
{
public:
  RgbMapImpl() {
    m_allegMap = new RGB_MAP;
    m_palette_id = 0;
    m_modifications = 0;
  }

  ~RgbMapImpl() {
    delete m_allegMap;
  }

  bool match(const Palette* palette) const {
    return (m_palette_id == palette->id &&
	    m_modifications == palette->getModifications());
  }

  void regenerate(const Palette* palette) {
    m_palette_id = palette->id;
    m_modifications = palette->getModifications();

    PALETTE allegPal;
    palette->toAllegro(allegPal);
    create_rgb_table(m_allegMap, allegPal, NULL);
  }

  int mapColor(int r, int g, int b) const {
    assert(r >= 0 && r < 256);
    assert(g >= 0 && g < 256);
    assert(b >= 0 && b < 256);
    return m_allegMap->data[r>>3][g>>3][b>>3];
  }

private:
  RGB_MAP* m_allegMap;
  gfxobj_id m_palette_id;
  size_t m_modifications;
};

//////////////////////////////////////////////////////////////////////
// RgbMap

RgbMap::RgbMap()
  : GfxObj(GFXOBJ_RGBMAP)
{
  m_impl = new RgbMapImpl;
}

RgbMap::~RgbMap()
{
  delete m_impl;
}

bool RgbMap::match(const Palette* palette) const
{
  return m_impl->match(palette);
}

void RgbMap::regenerate(const Palette* palette)
{
  m_impl->regenerate(palette);
}

int RgbMap::mapColor(int r, int g, int b) const
{
  return m_impl->mapColor(r, g, b);
}
