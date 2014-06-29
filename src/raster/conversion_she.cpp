/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/conversion_she.h"

#include "raster/algo.h"
#include "raster/blend.h"
#include "raster/color_scales.h"
#include "raster/image.h"
#include "raster/image_impl.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"
#include "she/surface.h"
#include "she/surface_format.h"
#include "she/scoped_surface_lock.h"

#include <stdexcept>

namespace raster {

namespace {

template<typename ImageTraits, she::SurfaceFormat format>
uint32_t convert_color_to_surface(color_t color, const Palette* palette, const she::SurfaceFormatData* fd) {
  // Error, it must use a specialization

  // TODO Use a static_assert(false)
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
void convert_image_to_surface_templ(const Image* image, she::LockedSurface* dst, int x, int y, const Palette* palette, const she::SurfaceFormatData* fd)
{
  const LockImageBits<ImageTraits> bits(image);
  typename LockImageBits<ImageTraits>::const_iterator src_it = bits.begin(), src_end = bits.end();
  int w = image->getWidth();
  int h = image->getHeight();

  for (int v=0; v<h; ++v) {
    AddressType dst_address = AddressType(dst->getData(x, y));
    for (int u=0; u<w; ++u) {
      ASSERT(src_it != src_end);

      *dst_address = convert_color_to_surface<ImageTraits, she::kRgbaSurfaceFormat>(*src_it, palette, fd);
      ++dst_address;
      ++src_it;
    }
    ++y;
  }
}

struct Address24bpp
{
  uint8_t* m_ptr;
  Address24bpp(uint8_t* ptr) : m_ptr(ptr) { }
  Address24bpp& operator++() { m_ptr += 3; return *this; }
  Address24bpp& operator*() { return *this; }
  Address24bpp& operator=(uint32_t c) {
    WRITE3BYTES(m_ptr, c);
    return *this;
  }
};

template<typename ImageTraits>
void convert_image_to_surface_selector(const Image* image, she::LockedSurface* surface, int x, int y, const Palette* palette, const she::SurfaceFormatData* fd)
{
  switch (fd->bitsPerPixel) {
    case 8: convert_image_to_surface_templ<ImageTraits, uint8_t*>(image, surface, x, y, palette, fd); break;
    case 15:
    case 16: convert_image_to_surface_templ<ImageTraits, uint16_t*>(image, surface, x, y, palette, fd); break;
    case 24: convert_image_to_surface_templ<ImageTraits, Address24bpp>(image, surface, x, y, palette, fd); break;
    case 32: convert_image_to_surface_templ<ImageTraits, uint32_t*>(image, surface, x, y, palette, fd); break;
  }
}

} // anonymous namespace

void convert_image_to_surface(const Image* image, she::Surface* surface, int x, int y, const Palette* palette)
{
  she::ScopedSurfaceLock dst(surface);
  she::SurfaceFormatData fd;
  dst->getFormat(&fd);

  switch (image->getPixelFormat()) {
    case IMAGE_RGB: convert_image_to_surface_selector<RgbTraits>(image, dst, x, y, palette, &fd); break;
    case IMAGE_GRAYSCALE: convert_image_to_surface_selector<GrayscaleTraits>(image, dst, x, y, palette, &fd); break;
    case IMAGE_INDEXED: convert_image_to_surface_selector<IndexedTraits>(image, dst, x, y, palette, &fd); break;
    case IMAGE_BITMAP: convert_image_to_surface_selector<BitmapTraits>(image, dst, x, y, palette, &fd); break;
    default:
      ASSERT(false);
      throw std::runtime_error("conversion not supported");
  }
}

} // namespace raster
