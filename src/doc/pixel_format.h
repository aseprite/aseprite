// Aseprite Document Library
// Copyright (c) 2019-2025 Igara Studio S.A.
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_PIXEL_FORMAT_H_INCLUDED
#define DOC_PIXEL_FORMAT_H_INCLUDED
#pragma once

// Use 1-bit per pixel in IMAGE_BITMAP
#define DOC_USE_BITMAP_AS_1BPP 1

// Use 8-bit per pixel in IMAGE_BITMAP
// #define DOC_USE_BITMAP_AS_1BPP 0

namespace doc {

// This enum might be replaced/deprecated/removed in the future, please use
// doc::ColorMode instead.
enum PixelFormat {
  IMAGE_RGB,       // 32bpp see doc::rgba()
  IMAGE_GRAYSCALE, // 16bpp see doc::graya()
  IMAGE_INDEXED,   // 8bpp
  IMAGE_BITMAP,    // 1bpp
  IMAGE_TILEMAP,   // 32bpp see doc::tile()
};

} // namespace doc

#endif
