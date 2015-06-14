// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGE_TRAITS_H_INCLUDED
#define DOC_IMAGE_TRAITS_H_INCLUDED
#pragma once

#include "doc/blend_funcs.h"
#include "doc/pixel_format.h"

namespace doc {

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

    static inline int getRowStrideBytes(int pixels_per_row) {
      return bytes_per_pixel * pixels_per_row;
    }

    static inline BlendFunc get_blender(BlendMode blend_mode) {
      return get_rgba_blender(blend_mode);
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

    static inline int getRowStrideBytes(int pixels_per_row) {
      return bytes_per_pixel * pixels_per_row;
    }

    static inline BlendFunc get_blender(BlendMode blend_mode) {
      return get_graya_blender(blend_mode);
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

    static inline int getRowStrideBytes(int pixels_per_row) {
      return bytes_per_pixel * pixels_per_row;
    }

    static inline BlendFunc get_blender(BlendMode blend_mode) {
      return get_indexed_blender(blend_mode);
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

    static inline int getRowStrideBytes(int pixels_per_row) {
      return ((pixels_per_row+7) / 8);
    }
  };

} // namespace doc

#endif
