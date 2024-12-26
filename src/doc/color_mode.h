// Aseprite Document Library
// Copyright (c) 2019-2023 Igara Studio S.A.
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_COLOR_MODE_H_INCLUDED
#define DOC_COLOR_MODE_H_INCLUDED
#pragma once

namespace doc {

enum class ColorMode {
  RGB,
  GRAYSCALE,
  INDEXED,
  BITMAP,
  TILEMAP,
};

inline constexpr int bytes_per_pixel_for_colormode(ColorMode cm)
{
  switch (cm) {
    case ColorMode::RGB:       return 4; // RgbTraits::bytes_per_pixel
    case ColorMode::GRAYSCALE: return 2; // GrayscaleTraits::bytes_per_pixel
    case ColorMode::INDEXED:   return 1; // IndexedTraits::bytes_per_pixel
    case ColorMode::BITMAP:    return 1; // BitmapTraits::bytes_per_pixel
    case ColorMode::TILEMAP:   return 4;   // TilemapTraits::bytes_per_pixel
  }
  return 0;
}

} // namespace doc

#endif
