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

#include <allegro.h>

#include "app/color_utils.h"
#include "app/color.h"
#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/palette.h"

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
      c = ui::rgba(_rgba_getr(_c),
                   _rgba_getg(_c),
                   _rgba_getb(_c), 255);
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
                           _rgba_getr(_c),
                           _rgba_getg(_c),
                           _rgba_getb(_c), 255);
      }
      break;

  }

  return c;
}

int color_utils::color_for_image(const app::Color& color, PixelFormat format)
{
  if (color.getType() == app::Color::MaskType)
    return 0;

  int c = -1;

  switch (format) {
    case IMAGE_RGB:
      c = _rgba(color.getRed(), color.getGreen(), color.getBlue(), 255);
      break;
    case IMAGE_GRAYSCALE:
      c = _graya(color.getGray(), 255);
      break;
    case IMAGE_INDEXED:
      if (color.getType() == app::Color::IndexType)
        c = color.getIndex();
      else
        c = get_current_palette()->findBestfit(color.getRed(), color.getGreen(), color.getBlue());
      break;
  }

  return c;
}

int color_utils::color_for_layer(const app::Color& color, Layer* layer)
{
  int pixel_color;

  if (color.getType() == app::Color::MaskType) {
    pixel_color = layer->getSprite()->getTransparentColor();
  }
  else {
    PixelFormat format = layer->getSprite()->getPixelFormat();
    pixel_color = color_for_image(color, format);
  }

  return fixup_color_for_layer(layer, pixel_color);
}

int color_utils::fixup_color_for_layer(Layer *layer, int color)
{
  if (layer->isBackground())
    return fixup_color_for_background(layer->getSprite()->getPixelFormat(), color);
  else
    return color;
}

int color_utils::fixup_color_for_background(PixelFormat format, int color)
{
  switch (format) {
    case IMAGE_RGB:
      if (_rgba_geta(color) < 255) {
        return _rgba(_rgba_getr(color),
                     _rgba_getg(color),
                     _rgba_getb(color), 255);
      }
      break;
    case IMAGE_GRAYSCALE:
      if (_graya_geta(color) < 255) {
        return _graya(_graya_getv(color), 255);
      }
      break;
  }
  return color;
}
