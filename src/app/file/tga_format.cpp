// Aseprite
// Copyright (C) 2019 Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
//
// tga.c - Based on the code of Tim Gunn, Michal Mertl, Salvador
//         Eduardo Tropea and Peter Wang.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "base/cfile.h"
#include "base/file_handle.h"
#include "doc/doc.h"

namespace app {

using namespace base;

class TgaFormat : public FileFormat {
  const char* onGetName() const override {
    return "tga";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("tga");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::TARGA_IMAGE;
  }

  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreateTgaFormat()
{
  return new TgaFormat;
}

/* rle_tga_read:
 *  Helper for reading 256 color RLE data from TGA files.
 */
static void rle_tga_read(unsigned char *address, int w, int type, FILE *f)
{
  unsigned char value;
  int count, g;
  int c = 0;

  do {
    count = fgetc(f);
    if (count & 0x80) {
      count = (count & 0x7F) + 1;
      c += count;
      value = fgetc(f);
      while (count--) {
        if (type == 1)
          *(address++) = value;
        else {
          *((uint16_t*)address) = value;
          address += sizeof(uint16_t);
        }
      }
    }
    else {
      count++;
      c += count;
      if (type == 1) {
        fread(address, 1, count, f);
        address += count;
      }
      else {
        for (g=0; g<count; g++) {
          *((uint16_t*)address) = fgetc(f);
          address += sizeof(uint16_t);
        }
      }
    }
  } while (c < w);
}

/* rle_tga_read32:
 *  Helper for reading 32 bit RLE data from TGA files.
 */
static void rle_tga_read32(uint32_t* address, int w, FILE *f)
{
  unsigned char value[4];
  int count;
  int c = 0;

  do {
    count = fgetc(f);
    if (count & 0x80) {
      count = (count & 0x7F) + 1;
      c += count;
      fread(value, 1, 4, f);
      while (count--)
        *(address++) = rgba(value[2], value[1], value[0], value[3]);
    }
    else {
      count++;
      c += count;
      while (count--) {
        fread(value, 1, 4, f);
        *(address++) = rgba(value[2], value[1], value[0], value[3]);
      }
    }
  } while (c < w);
}

/* rle_tga_read24:
 *  Helper for reading 24 bit RLE data from TGA files.
 */
static void rle_tga_read24(uint32_t* address, int w, FILE *f)
{
  unsigned char value[4];
  int count;
  int c = 0;

  do {
    count = fgetc(f);
    if (count & 0x80) {
      count = (count & 0x7F) + 1;
      c += count;
      fread(value, 1, 3, f);
      while (count--)
        *(address++) = rgba(value[2], value[1], value[0], 255);
    }
    else {
      count++;
      c += count;
      while (count--) {
        fread(value, 1, 3, f);
        *(address++) = rgba(value[2], value[1], value[0], 255);
      }
    }
  } while (c < w);
}

/* rle_tga_read16:
 *  Helper for reading 16 bit RLE data from TGA files.
 */
static void rle_tga_read16(uint32_t* address, int w, FILE *f)
{
  unsigned int value;
  uint32_t color;
  int count;
  int c = 0;

  do {
    count = fgetc(f);
    if (count & 0x80) {
      count = (count & 0x7F) + 1;
      c += count;
      value = fgetw(f);
      color = rgba(scale_5bits_to_8bits(((value >> 10) & 0x1F)),
                   scale_5bits_to_8bits(((value >> 5) & 0x1F)),
                   scale_5bits_to_8bits((value & 0x1F)), 255);

      while (count--)
        *(address++) = color;
    }
    else {
      count++;
      c += count;
      while (count--) {
        value = fgetw(f);
        color = rgba(scale_5bits_to_8bits(((value >> 10) & 0x1F)),
                     scale_5bits_to_8bits(((value >> 5) & 0x1F)),
                     scale_5bits_to_8bits((value & 0x1F)), 255);
        *(address++) = color;
      }
    }
  } while (c < w);
}

// Loads a 256 color or 24 bit uncompressed TGA file, returning a bitmap
// structure and storing the palette data in the specified palette (this
// should be an array of at least 256 RGB structures).
bool TgaFormat::onLoad(FileOp* fop)
{
  unsigned char image_id[256], image_palette[256][3], rgb[4];
  unsigned char id_length, palette_type, image_type, palette_entry_size;
  unsigned char bpp, descriptor_bits;
  short unsigned int palette_colors;
  short unsigned int image_width, image_height;
  unsigned int c, i, x, y, yc;
  int compressed;

  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();

  id_length = fgetc(f);
  palette_type = fgetc(f);
  image_type = fgetc(f);
  fgetw(f);                     // first_color
  palette_colors  = fgetw(f);
  palette_entry_size = fgetc(f);
  fgetw(f);                     // "left" field
  fgetw(f);                     // "top" field
  image_width = fgetw(f);
  image_height = fgetw(f);
  bpp = fgetc(f);
  descriptor_bits = fgetc(f);

  fread(image_id, 1, id_length, f);

  if (palette_type == 1) {
    for (i=0; i<palette_colors; i++) {
      switch (palette_entry_size) {

        case 16:
          c = fgetw(f);
          image_palette[i][0] = (c & 0x1F) << 3;
          image_palette[i][1] = ((c >> 5) & 0x1F) << 3;
          image_palette[i][2] = ((c >> 10) & 0x1F) << 3;
          break;

        case 24:
        case 32:
          image_palette[i][0] = fgetc(f);
          image_palette[i][1] = fgetc(f);
          image_palette[i][2] = fgetc(f);
          if (palette_entry_size == 32)
            fgetc(f);
          break;
      }
    }
  }
  else if (palette_type != 0) {
    return false;
  }

  /* Image type:
   *    0 = no image data
   *    1 = uncompressed color mapped
   *    2 = uncompressed true color
   *    3 = grayscale
   *    9 = RLE color mapped
   *   10 = RLE true color
   *   11 = RLE grayscale
   */
  compressed = (image_type & 8);
  image_type &= 7;

  PixelFormat pixelFormat;

  switch (image_type) {

    /* paletted image */
    case 1:
      if ((palette_type != 1) || (bpp != 8)) {
        return false;
      }

      for (i=0; i<palette_colors; i++) {
        fop->sequenceSetColor(i,
                              image_palette[i][2],
                              image_palette[i][1],
                              image_palette[i][0]);
      }

      pixelFormat = IMAGE_INDEXED;
      break;

    /* truecolor image */
    case 2:
      if ((palette_type != 0) ||
          ((bpp != 15) && (bpp != 16) &&
           (bpp != 24) && (bpp != 32))) {
        return false;
      }
      if ((descriptor_bits & 0xf) == 8)
        fop->sequenceSetHasAlpha(true);
      pixelFormat = IMAGE_RGB;
      break;

    /* grayscale image */
    case 3:
      if ((palette_type != 0) || (bpp != 8)) {
        return false;
      }

      for (i=0; i<256; i++)
        fop->sequenceSetColor(i, i, i, i);

      pixelFormat = IMAGE_GRAYSCALE;
      break;

    default:
      /* TODO add support for more TGA types? */
      return false;
  }

  Image* image = fop->sequenceImage(pixelFormat, image_width, image_height);
  if (!image)
    return false;

  for (y=image_height; y; y--) {
    yc = (descriptor_bits & 0x20) ? image_height-y : y-1;

    switch (image_type) {

      case 1:
      case 3:
        if (compressed)
          rle_tga_read(image->getPixelAddress(0, yc), image_width, image_type, f);
        else if (image_type == 1)
          fread(image->getPixelAddress(0, yc), 1, image_width, f);
        else {
          for (x=0; x<image_width; x++)
            put_pixel_fast<GrayscaleTraits>(image, x, yc, graya(fgetc(f), 255));
        }
        break;

      case 2:
        if (bpp == 32) {
          if (compressed) {
            rle_tga_read32((uint32_t*)image->getPixelAddress(0, yc), image_width, f);
          }
          else {
            for (x=0; x<image_width; x++) {
              fread(rgb, 1, 4, f);
              put_pixel_fast<RgbTraits>(image, x, yc, rgba(rgb[2], rgb[1], rgb[0], rgb[3]));
            }
          }
        }
        else if (bpp == 24) {
          if (compressed) {
            rle_tga_read24((uint32_t*)image->getPixelAddress(0, yc), image_width, f);
          }
          else {
            for (x=0; x<image_width; x++) {
              fread(rgb, 1, 3, f);
              put_pixel_fast<RgbTraits>(image, x, yc, rgba(rgb[2], rgb[1], rgb[0], 255));
            }
          }
        }
        else {
          if (compressed) {
            rle_tga_read16((uint32_t*)image->getPixelAddress(0, yc), image_width, f);
          }
          else {
            for (x=0; x<image_width; x++) {
              c = fgetw(f);
              put_pixel_fast<RgbTraits>(image, x, yc, rgba(((c >> 10) & 0x1F),
                                                           ((c >> 5) & 0x1F),
                                                           (c & 0x1F), 255));
            }
          }
        }
        break;
    }

    if (image_height > 1) {
      fop->setProgress((float)(image_height-y) / (float)(image_height));
      if (fop->isStop())
        break;
    }
  }

  if (ferror(f)) {
    fop->setError("Error reading file.\n");
    return false;
  }
  else {
    return true;
  }
}

#ifdef ENABLE_SAVE
// Writes a bitmap into a TGA file, using the specified palette (this
// should be an array of at least 256 RGB structures).
bool TgaFormat::onSave(FileOp* fop)
{
  const Image* image = fop->sequenceImage();
  unsigned char image_palette[256][3];
  int x, y, c, r, g, b;
  int depth = (image->pixelFormat() == IMAGE_RGB) ? 32 : 8;
  bool need_pal = (image->pixelFormat() == IMAGE_INDEXED)? true: false;

  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();

  fputc(0, f);                          /* id length (no id saved) */
  fputc((need_pal) ? 1 : 0, f);         /* palette type */
  /* image type */
  fputc((image->pixelFormat() == IMAGE_RGB      ) ? 2 :
        (image->pixelFormat() == IMAGE_GRAYSCALE) ? 3 :
        (image->pixelFormat() == IMAGE_INDEXED  ) ? 1 : 0, f);
  fputw(0, f);                         /* first colour */
  fputw((need_pal) ? 256 : 0, f);      /* number of colours */
  fputc((need_pal) ? 24 : 0, f);       /* palette entry size */
  fputw(0, f);                         /* left */
  fputw(0, f);                         /* top */
  fputw(image->width(), f);                  /* width */
  fputw(image->height(), f);                  /* height */
  fputc(depth, f);                     /* bits per pixel */

  /* descriptor (bottom to top, 8-bit alpha) */
  fputc(image->pixelFormat() == IMAGE_RGB && !fop->document()->sprite()->isOpaque()? 8: 0, f);

  if (need_pal) {
    for (y=0; y<256; y++) {
      fop->sequenceGetColor(y, &r, &g, &b);
      image_palette[y][2] = r;
      image_palette[y][1] = g;
      image_palette[y][0] = b;
    }
    fwrite(image_palette, 1, 768, f);
  }

  switch (image->pixelFormat()) {

    case IMAGE_RGB:
      for (y=image->height()-1; y>=0; y--) {
        for (x=0; x<image->width(); x++) {
          c = get_pixel(image, x, y);
          fputc(rgba_getb(c), f);
          fputc(rgba_getg(c), f);
          fputc(rgba_getr(c), f);
          fputc(rgba_geta(c), f);
        }

        fop->setProgress((float)(image->height()-y) / (float)(image->height()));
      }
      break;

    case IMAGE_GRAYSCALE:
      for (y=image->height()-1; y>=0; y--) {
        for (x=0; x<image->width(); x++)
          fputc(graya_getv(get_pixel(image, x, y)), f);

        fop->setProgress((float)(image->height()-y) / (float)(image->height()));
      }
      break;

    case IMAGE_INDEXED:
      for (y=image->height()-1; y>=0; y--) {
        for (x=0; x<image->width(); x++)
          fputc(get_pixel(image, x, y), f);

        fop->setProgress((float)(image->height()-y) / (float)(image->height()));
      }
      break;
  }

  const char* tga2_footer = "\0\0\0\0\0\0\0\0TRUEVISION-XFILE.\0";
  fwrite(tga2_footer, 1, 26, f);

  if (ferror(f)) {
    fop->setError("Error writing file.\n");
    return false;
  }
  else {
    return true;
  }
}
#endif

} // namespace app
