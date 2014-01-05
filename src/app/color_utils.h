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

#ifndef APP_COLOR_UTILS_H_INCLUDED
#define APP_COLOR_UTILS_H_INCLUDED

#include "app/color.h"
#include "raster/color.h"
#include "raster/pixel_format.h"
#include "ui/color.h"

namespace raster {
  class Layer;
}

namespace app {
  namespace color_utils {

    ui::Color blackandwhite(ui::Color color);
    ui::Color blackandwhite_neg(ui::Color color);

    ui::Color color_for_ui(const app::Color& color);
    int color_for_allegro(const app::Color& color, int depth);
    raster::color_t color_for_image(const app::Color& color, raster::PixelFormat format);
    raster::color_t color_for_layer(const app::Color& color, raster::Layer* layer);

    raster::color_t fixup_color_for_layer(raster::Layer* layer, raster::color_t color);
    raster::color_t fixup_color_for_background(raster::PixelFormat format, raster::color_t color);

  } // namespace color_utils
} // namespace app

#endif
