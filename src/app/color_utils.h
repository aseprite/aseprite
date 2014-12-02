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

#ifndef APP_COLOR_UTILS_H_INCLUDED
#define APP_COLOR_UTILS_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/color_target.h"
#include "gfx/color.h"
#include "raster/color.h"
#include "raster/pixel_format.h"

namespace raster {
  class Layer;
}

namespace app {
  namespace color_utils {

    gfx::Color blackandwhite(gfx::Color color);
    gfx::Color blackandwhite_neg(gfx::Color color);

    gfx::Color color_for_ui(const app::Color& color);
    raster::color_t color_for_image(const app::Color& color, raster::PixelFormat format);
    raster::color_t color_for_layer(const app::Color& color, raster::Layer* layer);
    raster::color_t color_for_target_mask(const app::Color& color, const ColorTarget& colorTarget);
    raster::color_t color_for_target(const app::Color& color, const ColorTarget& colorTarget);

  } // namespace color_utils
} // namespace app

#endif
