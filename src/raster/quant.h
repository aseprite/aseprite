/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#ifndef RASTER_QUANT_H
#define RASTER_QUANT_H

#include <allegro/color.h>

/* dithering methods */
enum {
  DITHERING_NONE,
  DITHERING_ORDERED,
};

struct Image;

struct Image *image_set_imgtype (struct Image *image, int imgtype,
				 int dithering_method,
				 RGB_MAP *rgb_map,
				 RGB *palette);

struct Image *image_rgb_to_indexed (struct Image *src_image,
				    int offsetx, int offsety,
				    RGB_MAP *rgb_map,
				    RGB *palette);

#endif /* RASTER_QUANT_H */
