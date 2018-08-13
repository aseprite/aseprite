// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/cfile.h"
#include "base/file_handle.h"
#include "doc/color_scales.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/primitives.h"

#include <cstdio>
#include <memory>

namespace app {

using namespace doc;

// Loads a PIC file (Animator and Animator Pro format)
Image* load_pic_file(const char* filename, int* x, int* y, Palette** palette)
{
  std::unique_ptr<Image> image;
  int size, compression;
  int block_size;
  int block_type;
  int version;
  int r, g, b;
  int c, u, v;
  int magic;
  int w, h;
  int byte;
  int bpp;

  base::FileHandle handle(base::open_file_with_exception(filename, "rb"));
  FILE* f = handle.get();

  // Animator format?
  magic = base::fgetw(f);
  if (magic == 0x9119) {
    // Read Animator PIC file
    w = base::fgetw(f);                 // width
    h = base::fgetw(f);                 // height
    *x = ((short)base::fgetw(f));       // X offset
    *y = ((short)base::fgetw(f));       // Y offset
    bpp = std::fgetc(f);                // bits per pixel (must be 8)
    compression = std::fgetc(f);        // compression flag (must be 0)
    base::fgetl(f);                     // image size (in bytes)
    std::fgetc(f);                      // reserved

    if (bpp != 8 || compression != 0) {
      return NULL;
    }

    // Read palette (RGB in 0-63)
    if (palette) {
      *palette = new Palette(frame_t(0), 256);
    }
    for (c=0; c<256; c++) {
      r = std::fgetc(f);
      g = std::fgetc(f);
      b = std::fgetc(f);
      if (palette)
        (*palette)->setEntry(c, rgba(
            scale_6bits_to_8bits(r),
            scale_6bits_to_8bits(g),
            scale_6bits_to_8bits(b), 255));
    }

    // Read image
    image.reset(Image::create(IMAGE_INDEXED, w, h));

    for (v=0; v<h; v++)
      for (u=0; u<w; u++)
        image->putPixel(u, v, std::fgetc(f));

    return image.release();
  }

  // rewind
  handle.reset();
  handle = base::open_file_with_exception(filename, "rb");
  f = handle.get();

  // read a PIC/MSK Animator Pro file
  size = base::fgetl(f);        // file size
  magic = base::fgetw(f);       // magic number 9500h
  if (magic != 0x9500)
    return NULL;

  w = base::fgetw(f);           // width
  h = base::fgetw(f);           // height
  *x = base::fgetw(f);          // X offset
  *y = base::fgetw(f);          // Y offset
  base::fgetl(f);               // user ID, is 0
  bpp = std::fgetc(f);          // bits per pixel

  if ((bpp != 1 && bpp != 8) || (w<1) || (h<1) || (w>9999) || (h>9999))
    return NULL;

  // Skip reserved data
  for (c=0; c<45; c++)
    std::fgetc(f);

  size -= 64;                   // The header uses 64 bytes

  image.reset(Image::create(bpp == 8 ? IMAGE_INDEXED: IMAGE_BITMAP, w, h));

  /* read blocks to end of file */
  while (size > 0) {
    block_size = base::fgetl(f);
    block_type = base::fgetw(f);

    switch (block_type) {

      // Color palette info
      case 0:
        version = base::fgetw(f);       // Palette version
        if (version != 0)
          return NULL;

        // 256 RGB entries in 0-255 format
        for (c=0; c<256; c++) {
          r = std::fgetc(f);
          g = std::fgetc(f);
          b = std::fgetc(f);
          if (palette)
            (*palette)->setEntry(c, rgba(r, g, b, 255));
        }
        break;

      // Byte-per-pixel image data
      case 1:
        for (v=0; v<h; v++)
          for (u=0; u<w; u++)
            image->putPixel(u, v, std::fgetc(f));
        break;

      // Bit-per-pixel image data
      case 2:
        for (v=0; v<h; v++)
          for (u=0; u<(w+7)/8; u++) {
            byte = std::fgetc(f);
            for (c=0; c<8; c++)
              put_pixel(image.get(), u*8+c, v, byte & (1<<(7-c)));
          }
        break;
    }

    size -= block_size;
  }

  return image.release();
}

// Saves an Animator Pro PIC file
int save_pic_file(const char *filename, int x, int y, const Palette* palette, const Image* image)
{
  int c, u, v, bpp, size, byte;

  if (image->pixelFormat() == IMAGE_INDEXED)
    bpp = 8;
  else if (image->pixelFormat() == IMAGE_BITMAP)
    bpp = 1;
  else
    return -1;

  if ((bpp == 8) && (!palette))
    return -1;

  base::FileHandle handle(base::open_file_with_exception_sync_on_close(filename, "wb"));
  FILE* f = handle.get();

  size = 64;
  // Bit-per-pixel image data block
  if (bpp == 1)
    size += (4+2+((image->width()+7)/8)*image->height());
  // Color palette info + byte-per-pixel image data block
  else
    size += (4+2+2+256*3) + (4+2+image->width()*image->height());

  base::fputl(size, f);          /* file size */
  base::fputw(0x9500, f);        /* magic number 9500h */
  base::fputw(image->width(), f);  /* width */
  base::fputw(image->height(), f); /* height */
  base::fputw(x, f);             /* X offset */
  base::fputw(y, f);             /* Y offset */
  base::fputl(0, f);             /* user ID, is 0 */
  std::fputc(bpp, f);            /* bits per pixel */

  // Reserved data
  for (c=0; c<45; c++)
    std::fputc(0, f);

  // 1 bpp
  if (bpp == 1) {
    // Bit-per-data image data block
    base::fputl((4+2+((image->width()+7)/8)*image->height()), f); // Block size
    base::fputw(2, f);                // Block type
    for (v=0; v<image->height(); v++) // Image data
      for (u=0; u<(image->width()+7)/8; u++) {
        byte = 0;
        for (c=0; c<8; c++)
          if (get_pixel(image, u*8+c, v))
            byte |= (1<<(7-c));
        std::fputc(byte, f);
      }
  }
  // 8 bpp
  else {
    ASSERT(palette);

    // Color palette info
    base::fputl((4+2+2+256*3), f);       // Block size
    base::fputw(0, f);                   // Block type
    base::fputw(0, f);                   // Version
    for (c=0; c<256; c++) {              // 256 palette entries
      color_t color = palette->getEntry(c);
      std::fputc(rgba_getr(color), f);
      std::fputc(rgba_getg(color), f);
      std::fputc(rgba_getb(color), f);
    }

    // Pixel-per-data image data block
    base::fputl((4+2+image->width()*image->height()), f); // Block size
    base::fputw(1, f);                // Block type
    for (v=0; v<image->height(); v++) // Image data
      for (u=0; u<image->width(); u++)
        std::fputc(image->getPixel(u, v), f);
  }

  return 0;
}

} // namespace app
