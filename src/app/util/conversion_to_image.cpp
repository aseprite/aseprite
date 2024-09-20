// Aseprite
// Copyright (c) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/conversion_to_image.h"

#include "doc/image_traits.h"
#include "doc/pixel_format.h"
#include "os/surface.h"

#include <memory>

namespace app {

using namespace doc;

// TODO: This implementation has a lot of room for improvement, I made the bare
// minimum to make it work. Right now it only supports converting RGBA surfaces,
// other kind of surfaces won't be converted to an image as expected.
void convert_surface_to_image(
  const os::Surface* surface,
  int src_x, int src_y,
  int w, int h,
  ImageRef& image)
{
  gfx::Rect srcBounds(src_x, src_y, w, h);
  srcBounds = srcBounds.createIntersection(surface->getClipBounds());
  if (srcBounds.isEmpty())
    return;

  src_x = srcBounds.x;
  src_y = srcBounds.y;
  w = srcBounds.w;
  h = srcBounds.h;

  image.reset(Image::create(PixelFormat::IMAGE_RGB, w, h));

  for (int v=0; v<h; ++v, ++src_y) {
    uint8_t* src_address = surface->getData(0, v);
    uint8_t* dst_address = image->getPixelAddress(src_x, src_y);
    std::copy(src_address,
              src_address + RgbTraits::bytes_per_pixel * w,
              dst_address);
  }
}

} // namespace app
