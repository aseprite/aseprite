// Aseprite
// Copyright (c) 2020  Igara Studio S.A.
// Copyright (c) 2001-2014 David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_CONVERSION_TO_SURFACE_H_INCLUDED
#define APP_UTIL_CONVERSION_TO_SURFACE_H_INCLUDED
#pragma once

namespace doc {
class Image;
class Palette;
} // namespace doc

namespace os {
class Surface;
}

namespace app {

void convert_image_to_surface(const doc::Image* image,
                              const doc::Palette* palette,
                              os::Surface* surface,
                              int src_x,
                              int src_y,
                              int dst_x,
                              int dst_y,
                              int w,
                              int h);

} // namespace app

#endif
