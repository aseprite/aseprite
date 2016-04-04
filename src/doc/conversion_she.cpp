// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/conversion_she.h"

#include "base/24bits.h"
#include "doc/algo.h"
#include "doc/color_scales.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"
#include "she/surface.h"
#include "she/surface_format.h"

#include <stdexcept>

namespace doc {

namespace {

template<typename ImageTraits, she::SurfaceFormat format>
uint32_t convert_color_to_surface(color_t color, const Palette* palette, const she::SurfaceFormatData* fd) {
  static_assert(false && sizeof(ImageTraits), "Invalid color conversion");
  return 0;
}

template<>
uint32_t convert_color_to_surface<RgbTraits, she::kRgbaSurfaceFormat>(color_t c, const Palette* palette, const she::SurfaceFormatData* fd) {
  return
    ((rgba_getr(c) << fd->redShift  ) & fd->redMask  ) |
    ((rgba_getg(c) << fd->greenShift) & fd->greenMask) |
    ((rgba_getb(c) << fd->blueShift ) & fd->blueMask ) |
    ((rgba_geta(c) << fd->alphaShift) & fd->alphaMask);
}

template<>
uint32_t convert_color_to_surface<GrayscaleTraits, she::kRgbaSurfaceFormat>(color_t c, const Palette* palette, const she::SurfaceFormatData* fd) {
  return
    ((graya_getv(c) << fd->redShift  ) & fd->redMask  ) |
    ((graya_getv(c) << fd->greenShift) & fd->greenMask) |
    ((graya_getv(c) << fd->blueShift ) & fd->blueMask ) |
    ((graya_geta(c) << fd->alphaShift) & fd->alphaMask);
}

template<>
uint32_t convert_color_to_surface<IndexedTraits, she::kRgbaSurfaceFormat>(color_t c0, const Palette* palette, const she::SurfaceFormatData* fd) {
  color_t c = palette->getEntry(c0);
  return
    ((rgba_getr(c) << fd->redShift  ) & fd->redMask  ) |
    ((rgba_getg(c) << fd->greenShift) & fd->greenMask) |
    ((rgba_getb(c) << fd->blueShift ) & fd->blueMask ) |
    ((rgba_geta(c) << fd->alphaShift) & fd->alphaMask);
}

template<>
uint32_t convert_color_to_surface<BitmapTraits, she::kRgbaSurfaceFormat>(color_t c0, const Palette* palette, const she::SurfaceFormatData* fd) {
  color_t c = palette->getEntry(c0);
  return
    ((rgba_getr(c) << fd->redShift  ) & fd->redMask  ) |
    ((rgba_getg(c) << fd->greenShift) & fd->greenMask) |
    ((rgba_getb(c) << fd->blueShift ) & fd->blueMask ) |
    ((rgba_geta(c) << fd->alphaShift) & fd->alphaMask);
}

template<typename ImageTraits, typename AddressType>
void convert_image_to_surface_templ(const Image* image, she::Surface* dst,
  int src_x, int src_y, int dst_x, int dst_y, int w, int h, const Palette* palette, const she::SurfaceFormatData* fd)
{
  const LockImageBits<ImageTraits> bits(image, gfx::Rect(src_x, src_y, w, h));
  typename LockImageBits<ImageTraits>::const_iterator src_it = bits.begin();
#ifdef _DEBUG
  typename LockImageBits<ImageTraits>::const_iterator src_end = bits.end();
#endif

  for (int v=0; v<h; ++v, ++dst_y) {
    AddressType dst_address = AddressType(dst->getData(dst_x, dst_y));
    for (int u=0; u<w; ++u) {
      ASSERT(src_it != src_end);

      *dst_address = convert_color_to_surface<ImageTraits, she::kRgbaSurfaceFormat>(*src_it, palette, fd);
      ++dst_address;
      ++src_it;
    }
  }
}

struct Address24bpp
{
  uint8_t* m_ptr;
  Address24bpp(uint8_t* ptr) : m_ptr(ptr) { }
  Address24bpp& operator++() { m_ptr += 3; return *this; }
  Address24bpp& operator*() { return *this; }
  Address24bpp& operator=(uint32_t c) {
    base::write24bits(m_ptr, c);
    return *this;
  }
};

template<typename ImageTraits>
void convert_image_to_surface_selector(const Image* image, she::Surface* surface,
  int src_x, int src_y, int dst_x, int dst_y, int w, int h, const Palette* palette, const she::SurfaceFormatData* fd)
{
  switch (fd->bitsPerPixel) {

    case 8:
      convert_image_to_surface_templ<ImageTraits, uint8_t*>(image, surface, src_x, src_y, dst_x, dst_y, w, h, palette, fd);
      break;

    case 15:
    case 16:
      convert_image_to_surface_templ<ImageTraits, uint16_t*>(image, surface, src_x, src_y, dst_x, dst_y, w, h, palette, fd);
      break;

    case 24:
      convert_image_to_surface_templ<ImageTraits, Address24bpp>(image, surface, src_x, src_y, dst_x, dst_y, w, h, palette, fd);
      break;

    case 32:
      convert_image_to_surface_templ<ImageTraits, uint32_t*>(image, surface, src_x, src_y, dst_x, dst_y, w, h, palette, fd);
      break;
  }
}

} // anonymous namespace

void convert_image_to_surface(const Image* image, const Palette* palette,
  she::Surface* surface, int src_x, int src_y, int dst_x, int dst_y, int w, int h)
{
  gfx::Rect srcBounds(src_x, src_y, w, h);
  srcBounds = srcBounds.createIntersection(image->bounds());
  if (srcBounds.isEmpty())
    return;

  src_x = srcBounds.x;
  src_y = srcBounds.y;
  w = srcBounds.w;
  h = srcBounds.h;

  gfx::Rect dstBounds(dst_x, dst_y, w, h);
  dstBounds = dstBounds.createIntersection(surface->getClipBounds());
  if (dstBounds.isEmpty())
    return;

  src_x += dstBounds.x - dst_x;
  src_y += dstBounds.y - dst_y;
  dst_x = dstBounds.x;
  dst_y = dstBounds.y;
  w = dstBounds.w;
  h = dstBounds.h;

  she::SurfaceLock lockDst(surface);
  she::SurfaceFormatData fd;
  surface->getFormat(&fd);

  switch (image->pixelFormat()) {

    case IMAGE_RGB:
      convert_image_to_surface_selector<RgbTraits>(image, surface, src_x, src_y, dst_x, dst_y, w, h, palette, &fd);
      break;

    case IMAGE_GRAYSCALE:
      convert_image_to_surface_selector<GrayscaleTraits>(image, surface, src_x, src_y, dst_x, dst_y, w, h, palette, &fd);
      break;

    case IMAGE_INDEXED:
      convert_image_to_surface_selector<IndexedTraits>(image, surface, src_x, src_y, dst_x, dst_y, w, h, palette, &fd);
      break;

    case IMAGE_BITMAP:
      convert_image_to_surface_selector<BitmapTraits>(image, surface, src_x, src_y, dst_x, dst_y, w, h, palette, &fd);
      break;

    default:
      ASSERT(false);
      throw std::runtime_error("conversion not supported");
  }
}

} // namespace doc
