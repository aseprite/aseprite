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

#ifndef APP_COLOR_SWATCHES_H_INCLUDED
#define APP_COLOR_SWATCHES_H_INCLUDED
#pragma once

#include <vector>

#include "app/color.h"

namespace app {

class ColorSwatches
{
public:
  ColorSwatches(const std::string& name);

  size_t size() const { return m_colors.size(); }

  const std::string& getName() const { return m_name; }
  void setName(std::string& name) { m_name = name; }

  void addColor(const Color& color);
  void insertColor(size_t index, const Color& color);
  void removeColor(size_t index);

  const Color& operator[](size_t index) const {
    return m_colors[index];
  }

private:
  std::string m_name;
  std::vector<Color> m_colors;
};

} // namespace app

#endif
