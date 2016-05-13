// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/image.h"

#include "doc/algo.h"
#include "doc/brush.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/rgbmap.h"

namespace doc {

Image::Image(PixelFormat format, int width, int height)
  : Object(ObjectType::Image)
  , m_spec((ColorMode)format, width, height, 0)
{
}

Image::~Image()
{
}

int Image::getMemSize() const
{
  return sizeof(Image) + getRowStrideSize()*height();
}

int Image::getRowStrideSize() const
{
  return getRowStrideSize(width());
}

int Image::getRowStrideSize(int pixels_per_row) const
{
  return calculate_rowstride_bytes(pixelFormat(), pixels_per_row);
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
  return crop_image(image, 0, 0, image->width(), image->height(),
    image->maskColor(), buffer);
}

} // namespace doc
