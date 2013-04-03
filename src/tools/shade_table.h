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

#ifndef TOOLS_SHADE_TABLE_H_INCLUDED
#define TOOLS_SHADE_TABLE_H_INCLUDED

#include <vector>
#include "tools/shading_mode.h"

namespace app { class ColorSwatches; }

namespace tools {

// Converts a ColorSwatches table to a temporary "shade table" used in
// shading ink so we can quickly rotate colors with left/right mouse
// buttons.
class ShadeTable8
{
public:
  ShadeTable8(const app::ColorSwatches& colorSwatches, ShadingMode mode);

  uint8_t left(uint8_t index) { return m_left[index]; }
  uint8_t right(uint8_t index) { return m_right[index]; }

private:
  std::vector<uint8_t> m_left;
  std::vector<uint8_t> m_right;
};

} // namespace tools

#endif  // TOOLS_SHADE_TABLE_H_INCLUDED
