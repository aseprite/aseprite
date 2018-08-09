// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CONVERSION_TO_SURFACE_H_INCLUDED
#define DOC_CONVERSION_TO_SURFACE_H_INCLUDED
#pragma once

namespace os {
  class Surface;
}

namespace doc {
  class Image;
  class Palette;

  void convert_image_to_surface(const Image* image, const Palette* palette,
    os::Surface* surface,
    int src_x, int src_y, int dst_x, int dst_y, int w, int h);

} // namespace doc

#endif
