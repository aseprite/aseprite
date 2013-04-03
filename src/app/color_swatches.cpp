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

#include "app/color_swatches.h"

namespace app {

ColorSwatches::ColorSwatches(const std::string& name)
  : m_name(name)
{
}

void ColorSwatches::addColor(Color& color)
{
  m_colors.push_back(color);
}

void ColorSwatches::insertColor(size_t index, Color& color)
{
  m_colors.insert(m_colors.begin() + index, color);
}

void ColorSwatches::removeColor(size_t index)
{
  m_colors.erase(m_colors.begin() + index);
}

} // namespace app
