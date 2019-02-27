// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_BUFFER_REGION_H_INCLUDED
#define APP_UTIL_BUFFER_REGION_H_INCLUDED
#pragma once

#include "base/buffer.h"
#include "gfx/fwd.h"

namespace doc {
  class Image;
}

namespace app {

  void save_image_region_in_buffer(
    const gfx::Region& region,
    const doc::Image* image,
    const gfx::Point& imagePos,
    base::buffer& buffer);

  void swap_image_region_with_buffer(
    const gfx::Region& region,
    doc::Image* image,
    base::buffer& buffer);

} // namespace app

#endif
