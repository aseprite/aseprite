/* ASEPRITE
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

#include "config.h"

#include "tools/shade_table.h"

#include "app/color_swatches.h"
#include "raster/palette.h"

namespace tools {

ShadeTable8::ShadeTable8(const app::ColorSwatches& colorSwatches, ShadingMode mode)
{
  m_left.resize(Palette::MaxColors);
  m_right.resize(Palette::MaxColors);

  for (size_t i=0; i<Palette::MaxColors; ++i) {
    m_left[i] = i;
    m_right[i] = i;
  }

  size_t n = colorSwatches.size();
  for (size_t i=0; i<n; ++i) {
    size_t leftDst;
    size_t rightDst;

    if (i == 0) {
      if (mode == kRotateShadingMode)
        leftDst = n-1;
      else
        leftDst = 0;
    }
    else
      leftDst = i-1;

    if (i == n-1) {
      if (mode == kRotateShadingMode)
        rightDst = 0;
      else
        rightDst = n-1;
    }
    else
      rightDst = i+1;

    size_t src = colorSwatches[i].getIndex();
    m_left[src] = colorSwatches[leftDst].getIndex();
    m_right[src] = colorSwatches[rightDst].getIndex();
  }
}

} // namespace tools
