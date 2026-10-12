// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/buffer_region.h"

#include "doc/image.h"
#include "gfx/region.h"

#include <algorithm>

namespace app {

void save_image_region_in_buffer(const gfx::Region& region,
                                 const doc::Image* image,
                                 const gfx::Point& imagePos,
                                 base::buffer& buffer)
{
  // Calculate buffer size for the region
  const size_t bytesPerPixel = image->bytesPerPixel();
  size_t reqBytes = 0;
  for (const auto& rc : region)
    reqBytes += bytesPerPixel * rc.w * rc.h;

  // Save region pixels
  buffer.resize(reqBytes);
  auto it = buffer.begin();
  for (const auto& rc : region) {
    for (int y = 0; y < rc.h; ++y) {
      ASSERT(it < buffer.end());

      auto p = (const uint8_t*)image->getPixelAddress(rc.x - imagePos.x, rc.y - imagePos.y + y);
      const size_t rowBytes = bytesPerPixel * rc.w;
      std::copy(p, p + rowBytes, it);
      it += rowBytes;
    }
  }
}

void swap_image_region_with_buffer(const gfx::Region& region,
                                   doc::Image* image,
                                   base::buffer& buffer)
{
  const size_t bytesPerPixel = image->bytesPerPixel();
  auto it = buffer.begin();
  const auto end = buffer.end();
  for (const auto& rc : region) {
    for (int y = 0; y < rc.h; ++y) {
      auto p = (uint8_t*)image->getPixelAddress(rc.x, rc.y + y);
      const size_t rowBytes = bytesPerPixel * rc.w;

      if (it + rowBytes > end) {
        LOG(ERROR, "Small buffer (%d bytes) to swap given region image\n", buffer.size());
        break;
      }

      std::swap_ranges(it, it + rowBytes, p);
      it += rowBytes;
    }
  }
}

} // namespace app
