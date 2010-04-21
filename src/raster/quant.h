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

#ifndef RASTER_QUANT_H_INCLUDED
#define RASTER_QUANT_H_INCLUDED

#include <allegro/color.h>

// Dithering methods
enum {
  DITHERING_NONE,
  DITHERING_ORDERED,
};

class Image;
class Palette;
class RgbMap;

Image* image_set_imgtype(const Image* image, int imgtype,
			 int dithering_method,
			 const RgbMap* rgbmap,
			 const Palette* palette);

Image* image_rgb_to_indexed(const Image* src_image,
			    int offsetx, int offsety,
			    const RgbMap* rgbmap,
			    const Palette* palette);

#endif
