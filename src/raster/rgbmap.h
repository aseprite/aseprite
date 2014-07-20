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

#ifndef RASTER_RGBMAP_H_INCLUDED
#define RASTER_RGBMAP_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "raster/object.h"

#include <vector>

namespace raster {

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

} // namespace raster

#endif
