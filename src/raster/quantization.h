/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef RASTER_QUANTIZATION_H_INCLUDED
#define RASTER_QUANTIZATION_H_INCLUDED

#include <allegro/color.h>
#include "raster/dithering_method.h"

class Image;
class Palette;
class RgbMap;
class Sprite;
class Stock;

namespace quantization {

  // Creates a new palette suitable to quantize the given RGB sprite to Indexed color.
  Palette* create_palette_from_rgb(const Sprite* sprite);

  // Changes the "imgtype" of the image. The dithering method is used
  // only when you want to convert from RGB to Indexed.
  Image* convert_imgtype(const Image* image, int imgtype,
			 DitheringMethod ditheringMethod,
			 const RgbMap* rgbmap,
			 const Palette* palette);

}

#endif
