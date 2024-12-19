// Aseprite
// Copyright (c) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/conversion_to_image.h"

#include "doc/pixel_format.h"
#include "os/surface.h"

#include <memory>

namespace app {

using namespace doc;

uint32_t convert_color_to_image(gfx::Color c, const os::SurfaceFormatData* fd)
{
  uint8_t r = ((c & fd->redMask) >> fd->redShift);
  uint8_t g = ((c & fd->greenMask) >> fd->greenShift);
  uint8_t b = ((c & fd->blueMask) >> fd->blueShift);
  uint8_t a = ((c & fd->alphaMask) >> fd->alphaShift);

  if (fd->pixelAlpha == os::PixelAlpha::kPremultiplied) {
    r = r * 255 / a;
    g = g * 255 / a;
    b = b * 255 / a;
  }

  return rgba(r, g, b, a);
}

// TODO: This implementation has a lot of room for improvement, I made the bare
// minimum to make it work. Right now it only supports converting RGBA surfaces,
// other kind of surfaces won't be converted to an image as expected.
void convert_surface_to_image(const os::Surface* surface,
                              int src_x,
                              int src_y,
                              int w,
                              int h,
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

  os::SurfaceFormatData fd;
  surface->getFormat(&fd);

  for (int v = 0; v < h; ++v) {
    for (int u = 0; u < w; ++u) {
      uint32_t* c = (uint32_t*)(surface->getData(u, v));
      image->putPixel(src_x + u, src_y + v, convert_color_to_image(*c, &fd));
    }
  }
}

} // namespace app
