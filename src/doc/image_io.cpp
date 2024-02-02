// Aseprite Document Library
// Copyright (c) 2019-2024  Igara Studio S.A.
// Copyright (c) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/image_io.h"

#include "base/exception.h"
#include "base/serialization.h"
#include "doc/cancel_io.h"
#include "doc/image.h"
#include "zlib.h"

#include <algorithm>
#include <iostream>
#include <memory>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

// TODO Create a zlib wrapper for iostreams

bool write_image(std::ostream& os, const Image* image, CancelIO* cancel)
{
  write32(os, image->id());
  write8(os, image->pixelFormat());    // Pixel format
  write16(os, image->width());         // Width
  write16(os, image->height());        // Height
  write32(os, image->maskColor());     // Mask color

  // Number of bytes for visible pixels on each row
  const int widthBytes = image->widthBytes();

#if 0
  {
    for (int c=0; c<image->height(); c++)
      os.write((char*)image->getPixelAddress(0, c), widthBytes);
  }
#else
  {
    std::ostream::pos_type total_output_pos = os.tellp();
    write32(os, 0);    // Compressed size (we update this value later)

    z_stream zstream;
    zstream.zalloc = (alloc_func)0;
    zstream.zfree  = (free_func)0;
    zstream.opaque = (voidpf)0;
    int err = deflateInit(&zstream, Z_DEFAULT_COMPRESSION);
    if (err != Z_OK)
      throw base::Exception("ZLib error %d in deflateInit().", err);

    std::vector<uint8_t> compressed(4096);
    int total_output_bytes = 0;

    for (int y=0; y<image->height(); y++) {
      if (cancel && cancel->isCanceled()) {
        deflateEnd(&zstream);
        return false;
      }

      zstream.next_in = (Bytef*)image->getPixelAddress(0, y);
      zstream.avail_in = widthBytes;
      int flush = (y == image->height()-1 ? Z_FINISH: Z_NO_FLUSH);

      do {
        zstream.next_out = (Bytef*)&compressed[0];
        zstream.avail_out = compressed.size();

        // Compress
        err = deflate(&zstream, flush);
        if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR)
          throw base::Exception("ZLib error %d in deflate().", err);

        int output_bytes = compressed.size() - zstream.avail_out;
        if (output_bytes > 0) {
          if (os.write((char*)&compressed[0], output_bytes).fail())
            throw base::Exception("Error writing compressed image pixels.\n");

          total_output_bytes += output_bytes;
        }
      } while (zstream.avail_out == 0);
    }

    err = deflateEnd(&zstream);
    if (err != Z_OK)
      throw base::Exception("ZLib error %d in deflateEnd().", err);

    std::ostream::pos_type bak = os.tellp();
    os.seekp(total_output_pos);
    write32(os, total_output_bytes);
    os.seekp(bak);
  }
#endif
  return true;
}

Image* read_image(std::istream& is, bool setId)
{
  ObjectId id = read32(is);
  int pixelFormat = read8(is);          // Pixel format
  int width = read16(is);               // Width
  int height = read16(is);              // Height
  uint32_t maskColor = read32(is);      // Mask color

  if ((pixelFormat != IMAGE_RGB &&
       pixelFormat != IMAGE_GRAYSCALE &&
       pixelFormat != IMAGE_INDEXED &&
       pixelFormat != IMAGE_BITMAP &&
       pixelFormat != IMAGE_TILEMAP) ||
      (width < 1 || height < 1) ||
      (width > 0xfffff || height > 0xfffff))
    return nullptr;

  std::unique_ptr<Image> image(
    Image::create(static_cast<PixelFormat>(pixelFormat), width, height));

  const int widthBytes = image->widthBytes();

#if 0
  {
    for (int c=0; c<image->height(); c++)
      is.read((char*)image->getPixelAddress(0, c), widthBytes);
  }
#else
  {
    int avail_bytes = read32(is);

    z_stream zstream;
    zstream.zalloc = (alloc_func)0;
    zstream.zfree  = (free_func)0;
    zstream.opaque = (voidpf)0;

    int err = inflateInit(&zstream);
    if (err != Z_OK)
      throw base::Exception("ZLib error %d in inflateInit().", err);

    int remain = avail_bytes;

    std::vector<uint8_t> compressed(4096);
    int y = 0;
    uint8_t* address = nullptr;
    uint8_t* address_end = nullptr;

    while (remain > 0) {
      int len = std::min(remain, int(compressed.size()));
      if (is.read((char*)&compressed[0], len).fail()) {
        ASSERT(false);
        throw base::Exception("Error reading stream to restore image");
      }

      int bytes_read = (int)is.gcount();
      if (bytes_read == 0) {
        ASSERT(remain == 0);
        break;
      }

      remain -= bytes_read;

      zstream.next_in = (Bytef*)&compressed[0];
      zstream.avail_in = (uInt)bytes_read;

      do {
        if (address == address_end) {
          if (y < image->height()) {
            address = image->getPixelAddress(0, y++);
            address_end = address + widthBytes;
          }
          else {
            // Special reported case where we just fill the whole
            // output image buffer (avail_out == 0), and more input
            // was previously reported as available (avail_in != 0).
            //
            // Not sure why zlib reports this in certain cases, where
            // avail_in != 0 and err == Z_OK instead of err ==
            // Z_STREAM_END and we have to do a final inflate() call
            // (even w/avail_out=0) to get the final Z_STREAM_END
            // result.
            ASSERT(y == image->height());
            ASSERT(err == Z_OK);
          }
        }

        zstream.next_out = (Bytef*)address;
        zstream.avail_out = address_end - address;

        err = inflate(&zstream, Z_NO_FLUSH);
        if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR)
          throw base::Exception("ZLib error %d in inflate().", err);

        int uncompressed_bytes = (int)((address_end - address) - zstream.avail_out);
        if (uncompressed_bytes > 0) {
          address += uncompressed_bytes;
        }
      } while (zstream.avail_in != 0 && zstream.avail_out == 0);
    }

    err = inflateEnd(&zstream);
    if (err != Z_OK)
      throw base::Exception("ZLib error %d in inflateEnd().", err);
  }
#endif

  image->setMaskColor(maskColor);
  if (setId)
    image->setId(id);
  return image.release();
}

}
