/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#include "raster/image.h"

#include "raster/algo.h"
#include "raster/blend.h"
#include "raster/image_impl.h"
#include "raster/palette.h"
#include "raster/pen.h"
#include "raster/primitives.h"
#include "raster/rgbmap.h"

namespace raster {

Image::Image(PixelFormat format, int width, int height)
  : Object(OBJECT_IMAGE)
  , m_format(format)
{
  m_width = width;
  m_height = height;
  m_maskColor = 0;
}

Image::~Image()
{
}

int Image::getMemSize() const
{
  return sizeof(Image) + getRowStrideSize()*m_height;
}

int Image::getRowStrideSize() const
{
  return getRowStrideSize(m_width);
}

int Image::getRowStrideSize(int pixels_per_row) const
{
  return calculate_rowstride_bytes(getPixelFormat(), m_width);
}

// static
Image* Image::create(PixelFormat format, int width, int height,
                     const ImageBufferPtr& buffer)
{
  switch (format) {
    case IMAGE_RGB:       return new ImageImpl<RgbTraits>(width, height, buffer);
    case IMAGE_GRAYSCALE: return new ImageImpl<GrayscaleTraits>(width, height, buffer);
    case IMAGE_INDEXED:   return new ImageImpl<IndexedTraits>(width, height, buffer);
    case IMAGE_BITMAP:    return new ImageImpl<BitmapTraits>(width, height, buffer);
  }
  return NULL;
}

// static
Image* Image::createCopy(const Image* image, const ImageBufferPtr& buffer)
{
  ASSERT(image);
  return crop_image(image, 0, 0, image->getWidth(), image->getHeight(), 0, buffer);
}

} // namespace raster
