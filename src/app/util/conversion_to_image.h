// Aseprite
// Copyright (c) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_CONVERSION_TO_IMAGE_H_INCLUDED
#define APP_UTIL_CONVERSION_TO_IMAGE_H_INCLUDED
#pragma once

#include "doc/image_ref.h"

namespace os {
class Surface;
}

namespace doc {
class Palette;
}

namespace app {

void convert_surface_to_image(const os::Surface* surface,
                              int src_x,
                              int src_y,
                              int w,
                              int h,
                              doc::ImageRef& image);

} // namespace app

#endif
