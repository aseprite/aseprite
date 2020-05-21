// Aseprite Document Library
// Copyright (c) 2018-2020 Igara Studio S.A.
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

Image::Image(const ImageSpec& spec)
  : Object(ObjectType::Image)
  , m_spec(spec)
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
  return Image::create(ImageSpec((ColorMode)format, width, height, 0), buffer);
}

// static
Image* Image::create(const ImageSpec& spec,
                     const ImageBufferPtr& buffer)
{
  ASSERT(spec.width() >= 1 && spec.height() >= 1);
  if (spec.width() < 1 || spec.height() < 1)
    return nullptr;

  switch (spec.colorMode()) {
    case ColorMode::RGB:       return new ImageImpl<RgbTraits>(spec, buffer);
    case ColorMode::GRAYSCALE: return new ImageImpl<GrayscaleTraits>(spec, buffer);
    case ColorMode::INDEXED:   return new ImageImpl<IndexedTraits>(spec, buffer);
    case ColorMode::BITMAP:    return new ImageImpl<BitmapTraits>(spec, buffer);
  }
  return nullptr;
}

// static
Image* Image::createCopy(const Image* image, const ImageBufferPtr& buffer)
{
  ASSERT(image);
  return crop_image(image, 0, 0, image->width(), image->height(),
                    image->maskColor(), buffer);
}

} // namespace doc
