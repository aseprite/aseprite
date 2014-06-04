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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>

#include "app/color.h"
#include "app/color_utils.h"
#include "app/modules/palettes.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"

using namespace gfx;

// Internal functions
namespace {

// Returns the same values of bitmap_mask_color() (this function *must*
// returns the same values).
int get_mask_for_bitmap(int depth)
{
  switch (depth) {
    case  8: return MASK_COLOR_8;  break;
    case 15: return MASK_COLOR_15; break;
    case 16: return MASK_COLOR_16; break;
    case 24: return MASK_COLOR_24; break;
    case 32: return MASK_COLOR_32; break;
    default:
      return -1;
  }
}

}

namespace app {

ui::Color color_utils::blackandwhite(ui::Color color)
{
  if ((ui::getr(color)*30+ui::getg(color)*59+ui::getb(color)*11)/100 < 128)
    return ui::rgba(0, 0, 0);
  else
    return ui::rgba(255, 255, 255);
}

ui::Color color_utils::blackandwhite_neg(ui::Color color)
{
  if ((ui::getr(color)*30+ui::getg(color)*59+ui::getb(color)*11)/100 < 128)
    return ui::rgba(255, 255, 255);
  else
    return ui::rgba(0, 0, 0);
}

ui::Color color_utils::color_for_ui(const app::Color& color)
{
  ui::Color c = ui::ColorNone;

  switch (color.getType()) {

    case app::Color::MaskType:
      c = ui::ColorNone;
      break;

    case app::Color::RgbType:
    case app::Color::HsvType:
      c = ui::rgba(color.getRed(),
                   color.getGreen(),
                   color.getBlue(), 255);
      break;

    case app::Color::GrayType:
      c = ui::rgba(color.getGray(),
                   color.getGray(),
                   color.getGray(), 255);
      break;

    case app::Color::IndexType: {
      int i = color.getIndex();
      ASSERT(i >= 0 && i < (int)get_current_palette()->size());

      uint32_t _c = get_current_palette()->getEntry(i);
      c = ui::rgba(rgba_getr(_c),
                   rgba_getg(_c),
                   rgba_getb(_c), 255);
      break;
    }

  }

  return c;
}

int color_utils::color_for_allegro(const app::Color& color, int depth)
{
  int c = -1;

  switch (color.getType()) {

    case app::Color::MaskType:
      c = get_mask_for_bitmap(depth);
      break;

    case app::Color::RgbType:
    case app::Color::HsvType:
      c = makeacol_depth(depth,
                         color.getRed(),
                         color.getGreen(),
                         color.getBlue(), 255);
      break;

    case app::Color::GrayType:
      c = color.getGray();
      if (depth != 8)
        c = makeacol_depth(depth, c, c, c, 255);
      break;

    case app::Color::IndexType:
      c = color.getIndex();
      if (depth != 8) {
        ASSERT(c >= 0 && c < (int)get_current_palette()->size());

        uint32_t _c = get_current_palette()->getEntry(c);
        c = makeacol_depth(depth,
                           rgba_getr(_c),
                           rgba_getg(_c),
                           rgba_getb(_c), 255);
      }
      break;

  }

  return c;
}

raster::color_t color_utils::color_for_image(const app::Color& color, PixelFormat format)
{
  if (color.getType() == app::Color::MaskType)
    return 0;

  raster::color_t c = -1;

  switch (format) {
    case IMAGE_RGB:
      c = rgba(color.getRed(), color.getGreen(), color.getBlue(), 255);
      break;
    case IMAGE_GRAYSCALE:
      c = graya(color.getGray(), 255);
      break;
    case IMAGE_INDEXED:
      c = color.getIndex();
      break;
  }

  return c;
}

raster::color_t color_utils::color_for_layer(const app::Color& color, Layer* layer)
{
  return color_for_target(color, ColorTarget(layer));
}

raster::color_t color_utils::color_for_target(const app::Color& color, const ColorTarget& colorTarget)
{
  if (color.getType() == app::Color::MaskType)
    return colorTarget.maskColor();

  raster::color_t c = -1;

  switch (colorTarget.pixelFormat()) {
    case IMAGE_RGB:
      c = rgba(color.getRed(), color.getGreen(), color.getBlue(), 255);
      break;
    case IMAGE_GRAYSCALE:
      c = graya(color.getGray(), 255);
      break;
    case IMAGE_INDEXED:
      if (color.getType() == app::Color::IndexType) {
        c = color.getIndex();
      }
      else {
        c = get_current_palette()->findBestfit(
          color.getRed(),
          color.getGreen(),
          color.getBlue(),
          colorTarget.isTransparent() ?
            colorTarget.maskColor(): // Don't return the mask color
            -1);                     // Return any color, we are in a background layer.
      }
      break;
  }

  return c;
}

} // namespace app
