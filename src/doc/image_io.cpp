// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/image_io.h"

#include "base/serialization.h"
#include "base/unique_ptr.h"
#include "doc/image.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// Serialized Image data:
//
//    DWORD             image ID
//    BYTE              image type
//    WORD[2]           w, h
//    DWORD             mask color
//    for each line     ("h" times)
//      for each pixel  ("w" times)
//        BYTE[4]       for RGB images, or
//        BYTE[2]       for Grayscale images, or
//        BYTE          for Indexed images

void write_image(std::ostream& os, const Image* image)
{
  write32(os, image->id());
  write8(os, image->pixelFormat());    // Pixel format
  write16(os, image->width());         // Width
  write16(os, image->height());        // Height
  write32(os, image->maskColor());     // Mask color

  int size = image->getRowStrideSize();
  for (int c=0; c<image->height(); c++)
    os.write((char*)image->getPixelAddress(0, c), size);
}

Image* read_image(std::istream& is)
{
  ObjectId id = read32(is);
  int pixelFormat = read8(is);          // Pixel format
  int width = read16(is);               // Width
  int height = read16(is);              // Height
  uint32_t maskColor = read32(is);      // Mask color

  base::UniquePtr<Image> image(Image::create(static_cast<PixelFormat>(pixelFormat), width, height));
  int size = image->getRowStrideSize();

  for (int c=0; c<image->height(); c++)
    is.read((char*)image->getPixelAddress(0, c), size);

  image->setMaskColor(maskColor);
  image->setId(id);
  return image.release();
}

}
