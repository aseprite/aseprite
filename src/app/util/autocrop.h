// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UTIL_AUTOCROP_H_INCLUDED
#define APP_UTIL_AUTOCROP_H_INCLUDED
#pragma once

#include "doc/color.h"

namespace doc {
  class Image;
}

namespace app {

  bool get_shrink_rect(int* x1, int* y1, int* x2, int* y2,
                       doc::Image *image, doc::color_t refpixel);
  bool get_shrink_rect2(int* x1, int* y1, int* x2, int* y2,
                        doc::Image* image, doc::Image* regimage);

} // namespace app

#endif
