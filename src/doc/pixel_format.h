// Aseprite Document Library
// Copyright (c) 2019 Igara Studio S.A.
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_PIXEL_FORMAT_H_INCLUDED
#define DOC_PIXEL_FORMAT_H_INCLUDED
#pragma once

namespace doc {

enum PixelFormat {
  IMAGE_RGB,       // 32bpp see doc::rgba()
  IMAGE_GRAYSCALE, // 16bpp see doc::graya()
  IMAGE_INDEXED,   // 8bpp
  IMAGE_BITMAP,    // 1bpp
  IMAGE_TILEMAP,   // 32bpp see doc::tile()
};

} // namespace doc

#endif
