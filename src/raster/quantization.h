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

#ifndef RASTER_QUANTIZATION_H_INCLUDED
#define RASTER_QUANTIZATION_H_INCLUDED
#pragma once

#include "raster/dithering_method.h"
#include "raster/frame_number.h"
#include "raster/pixel_format.h"

#include <vector>

namespace raster {

  class Image;
  class Palette;
  class RgbMap;
  class Sprite;
  class Stock;

  namespace quantization {

    void create_palette_from_images(
      const std::vector<Image*>& images,
      Palette* palette,
      bool has_background_layer);

    // Creates a new palette suitable to quantize the given RGB sprite to Indexed color.
    Palette* create_palette_from_rgb(
      const Sprite* sprite,
      FrameNumber frameNumber,
      Palette* newPalette);     // Can be NULL to create a new palette

    // Changes the image pixel format. The dithering method is used only
    // when you want to convert from RGB to Indexed.
    Image* convert_pixel_format(
      const Image* src,
      Image* dst,         // Can be NULL to create a new image
      PixelFormat pixelFormat,
      DitheringMethod ditheringMethod,
      const RgbMap* rgbmap,
      const Palette* palette,
      bool is_background);

  } // namespace quantization
} // namespace raster

#endif
