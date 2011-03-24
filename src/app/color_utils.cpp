/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

int color_utils::blackandwhite(int r, int g, int b)
{
  return (r*30+g*59+b*11)/100 < 128 ?
    makecol(0, 0, 0):
    makecol(255, 255, 255);
}

int color_utils::blackandwhite_neg(int r, int g, int b)
{
  return (r*30+g*59+b*11)/100 < 128 ?
    makecol(255, 255, 255):
    makecol(0, 0, 0);
}

int color_utils::color_for_allegro(const Color& color, int depth)
{
  int c = -1;
  
  switch (color.getType()) {

    case Color::MaskType:
      c = get_mask_for_bitmap(depth);
      break;

    case Color::RgbType:
    case Color::HsvType:
      c = makeacol_depth(depth,
			 color.getRed(),
			 color.getGreen(),
			 color.getBlue(), 255);
      break;

    case Color::GrayType:
      c = color.getGray();
      if (depth != 8)
	c = makeacol_depth(depth, c, c, c, 255);
      break;

    case Color::IndexType:
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

int color_utils::color_for_image(const Color& color, int imgtype)
{
  if (color.getType() == Color::MaskType)
    return 0;

  int c = -1;

  switch (imgtype) {
    case IMAGE_RGB:
      c = _rgba(color.getRed(), color.getGreen(), color.getBlue(), 255);
      break;
    case IMAGE_GRAYSCALE:
      c = _graya(color.getGray(), 255);
      break;
    case IMAGE_INDEXED:
      if (color.getType() == Color::IndexType)
	c = color.getIndex();
      else
	c = get_current_palette()->findBestfit(color.getRed(), color.getGreen(), color.getBlue());
      break;
  }

  return c;
}

int color_utils::color_for_layer(const Color& color, Layer* layer)
{
  int imgtype = layer->getSprite()->getImgType();

  return fixup_color_for_layer(layer, color_for_image(color, imgtype));
}

int color_utils::fixup_color_for_layer(Layer *layer, int color)
{
  if (layer->is_background())
    return fixup_color_for_background(layer->getSprite()->getImgType(), color);
  else
    return color;
}

int color_utils::fixup_color_for_background(int imgtype, int color)
{
  switch (imgtype) {
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
