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

#ifndef RASTER_IMAGE_TRAITS_H_INCLUDED
#define RASTER_IMAGE_TRAITS_H_INCLUDED

#include "raster/pixel_format.h"

namespace raster {

  struct RgbTraits {
    static const PixelFormat pixel_format = IMAGE_RGB;

    enum {
      bits_per_pixel = 32,
      bytes_per_pixel = 4,
      pixels_per_byte = 0,
      channels = 4,
      has_alpha = true,
    };

    typedef uint32_t pixel_t;
    typedef pixel_t* address_t;
    typedef const pixel_t* const_address_t;

    static const pixel_t min_value = 0x00000000l;
    static const pixel_t max_value = 0xffffffffl;
    
    static inline int getRowStrideBytes(int pixels_per_row)
    {
      return bytes_per_pixel * pixels_per_row;
    }

    static inline BLEND_COLOR get_blender(int blend_mode)
    {
      ASSERT(blend_mode >= 0 && blend_mode < BLEND_MODE_MAX);
      return rgba_blenders[blend_mode];
    }
  };

  struct GrayscaleTraits {
    static const PixelFormat pixel_format = IMAGE_GRAYSCALE;

    enum {
      bits_per_pixel = 16,
      bytes_per_pixel = 2,
      pixels_per_byte = 0,
      channels = 2,
      has_alpha = true,
    };

    typedef uint16_t pixel_t;
    typedef pixel_t* address_t;
    typedef const pixel_t* const_address_t;

    static const pixel_t min_value = 0x0000;
    static const pixel_t max_value = 0xffff;

    static inline int getRowStrideBytes(int pixels_per_row)
    {
      return bytes_per_pixel * pixels_per_row;
    }

    static inline BLEND_COLOR get_blender(int blend_mode)
    {
      ASSERT(blend_mode >= 0 && blend_mode < BLEND_MODE_MAX);
      return graya_blenders[blend_mode];
    }
  };

  struct IndexedTraits {
    static const PixelFormat pixel_format = IMAGE_INDEXED;

    enum {
      bits_per_pixel = 8,
      bytes_per_pixel = 1,
      pixels_per_byte = 1,
      channels = 1,
      has_alpha = false,
    };

    typedef uint8_t pixel_t;
    typedef pixel_t* address_t;
    typedef const pixel_t* const_address_t;

    static const pixel_t min_value = 0x00;
    static const pixel_t max_value = 0xff;

    static inline int getRowStrideBytes(int pixels_per_row)
    {
      return bytes_per_pixel * pixels_per_row;
    }
  };

  struct BitmapTraits {
    static const PixelFormat pixel_format = IMAGE_BITMAP;

    enum {
      bits_per_pixel = 1,
      bytes_per_pixel = 1,
      pixels_per_byte = 8,
      channels = 1,
      has_alpha = false,
    };

    typedef uint8_t pixel_t;
    typedef pixel_t* address_t;
    typedef const pixel_t* const_address_t;

    static const pixel_t min_value = 0;
    static const pixel_t max_value = 1;

    static inline int getRowStrideBytes(int pixels_per_row)
    {
      return ((pixels_per_row+7) / 8);
    }
  };

} // namespace raster

#endif
