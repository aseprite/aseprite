/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_UTIL_RANGE_UTILS_H_INCLUDED
#define APP_UTIL_RANGE_UTILS_H_INCLUDED
#pragma once

#include <vector>

namespace raster {
  class Sprite;
}

namespace app {
  class DocumentRange;

  std::vector<Cel*> get_cels_in_range(raster::Sprite* sprite, const DocumentRange& range);
  
} // namespace app

#endif
