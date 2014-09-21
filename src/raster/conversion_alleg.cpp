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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/conversion_alleg.h"

#include "raster/algo.h"
#include "raster/blend.h"
#include "raster/color_scales.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"

#include <allegro.h>

namespace raster {

void convert_palette_to_allegro(const Palette* palette, RGB* rgb)
{
  int i;
  for (i=0; i<palette->size(); ++i) {
    color_t c = palette->getEntry(i);
    rgb[i].r = rgba_getr(c) / 4;
    rgb[i].g = rgba_getg(c) / 4;
    rgb[i].b = rgba_getb(c) / 4;
  }
  for (; i<Palette::MaxColors; ++i) {
    rgb[i].r = 0;
    rgb[i].g = 0;
    rgb[i].b = 0;
  }
}

void convert_palette_from_allegro(const RGB* rgb, Palette* palette)
{
  palette->resize(Palette::MaxColors);
  for (int i=0; i<Palette::MaxColors; ++i) {
    palette->setEntry(i, rgba(scale_6bits_to_8bits(rgb[i].r),
                              scale_6bits_to_8bits(rgb[i].g),
                              scale_6bits_to_8bits(rgb[i].b), 255));
  }
}

} // namespace raster
