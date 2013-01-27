/* ASEPRITE
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
 *
 * tga.c - Based on the code of Tim Gunn, Michal Mertl, Salvador
 *         Eduardo Tropea and Peter Wang.
 */

#include "config.h"

#include "file/file.h"
#include "file/file_format.h"
#include "file/file_handle.h"
#include "file/format_options.h"
#include "raster/raster.h"

#include <allegro/color.h>

class TgaFormat : public FileFormat
{
  const char* onGetName() const { return "tga"; }
  const char* onGetExtensions() const { return "tga"; }
  int onGetFlags() const {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES;
  }

  bool onLoad(FileOp* fop);
  bool onSave(FileOp* fop);
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
static void rle_tga_read32 (uint32_t* address, int w, FILE *f)
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
        *(address++) = _rgba(value[2], value[1], value[0], value[3]);
    }
    else {
      count++;
      c += count;
      while (count--) {
        fread(value, 1, 4, f);
        *(address++) = _rgba(value[2], value[1], value[0], value[3]);
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
        *(address++) = _rgba(value[2], value[1], value[0], 255);
    }
    else {
      count++;
      c += count;
      while (count--) {
        fread(value, 1, 3, f);
        *(address++) = _rgba(value[2], value[1], value[0], 255);
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
      color = _rgba(_rgb_scale_5[((value >> 10) & 0x1F)],
                    _rgb_scale_5[((value >> 5) & 0x1F)],
                    _rgb_scale_5[(value & 0x1F)], 255);

      while (count--)
        *(address++) = color;
    }
    else {
      count++;
      c += count;
      while (count--) {
        value = fgetw(f);
        color = _rgba(_rgb_scale_5[((value >> 10) & 0x1F)],
                      _rgb_scale_5[((value >> 5) & 0x1F)],
                      _rgb_scale_5[(value & 0x1F)], 255);
        *(address++) = color;
      }
    }
  } while (c < w);
}

/* load_tga:
 *  Loads a 256 color or 24 bit uncompressed TGA file, returning a bitmap
 *  structure and storing the palette data in the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
bool TgaFormat::onLoad(FileOp* fop)
{
  unsigned char image_id[256], image_palette[256][3], rgb[4];
  unsigned char id_length, palette_type, image_type, palette_entry_size;
  unsigned char bpp, descriptor_bits;
  short unsigned int first_color, palette_colors;
  short unsigned int left, top, image_width, image_height;
  unsigned int c, i, x, y, yc;
  Image *image;
  int compressed;

  FileHandle f(fop->filename.c_str(), "rb");

  id_length = fgetc(f);
  palette_type = fgetc(f);
  image_type = fgetc(f);
  first_color = fgetw(f);
  palette_colors  = fgetw(f);
  palette_entry_size = fgetc(f);
  left = fgetw(f);
  top = fgetw(f);
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
        fop_sequence_set_color(fop, i,
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

      pixelFormat = IMAGE_RGB;
      break;

    /* grayscale image */
    case 3:
      if ((palette_type != 0) || (bpp != 8)) {
        return false;
      }

      for (i=0; i<256; i++)
        fop_sequence_set_color(fop, i, i, i, i);

      pixelFormat = IMAGE_GRAYSCALE;
      break;

    default:
      /* TODO add support for more TGA types? */
      return false;
  }

  image = fop_sequence_image(fop, pixelFormat, image_width, image_height);
  if (!image)
    return false;

  for (y=image_height; y; y--) {
    yc = (descriptor_bits & 0x20) ? image_height-y : y-1;

    switch (image_type) {

      case 1:
      case 3:
        if (compressed)
          rle_tga_read(image->line[yc], image_width, image_type, f);
        else if (image_type == 1)
          fread(image->line[yc], 1, image_width, f);
        else {
          for (x=0; x<image_width; x++)
            *(((uint16_t**)image->line)[yc]+x) =
              _graya(fgetc(f), 255);
        }
        break;

      case 2:
        if (bpp == 32) {
          if (compressed) {
            rle_tga_read32((uint32_t*)image->line[yc], image_width, f);
          }
          else {
            for (x=0; x<image_width; x++) {
              fread(rgb, 1, 4, f);
              *(((uint32_t**)image->line)[yc]+x) =
                _rgba(rgb[2], rgb[1], rgb[0], rgb[3]);
            }
          }
        }
        else if (bpp == 24) {
          if (compressed) {
            rle_tga_read24((uint32_t*)image->line[yc], image_width, f);
          }
          else {
            for (x=0; x<image_width; x++) {
              fread(rgb, 1, 3, f);
              *(((uint32_t**)image->line)[yc]+x) =
                _rgba(rgb[2], rgb[1], rgb[0], 255);
            }
          }
        }
        else {
          if (compressed) {
            rle_tga_read16((uint32_t*)image->line[yc], image_width, f);
          }
          else {
            for (x=0; x<image_width; x++) {
              c = fgetw(f);
              *(((uint32_t**)image->line)[yc]+x) =
                _rgba(((c >> 10) & 0x1F),
                      ((c >> 5) & 0x1F),
                      (c & 0x1F), 255);
            }
          }
        }
        break;
    }

    if (image_height > 1) {
      fop_progress(fop, (float)(image_height-y) / (float)(image_height));
      if (fop_is_stop(fop))
        break;
    }
  }

  if (ferror(f)) {
    fop_error(fop, "Error reading file.\n");
    return false;
  }
  else {
    return true;
  }
}

/* save_tga:
 *  Writes a bitmap into a TGA file, using the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
bool TgaFormat::onSave(FileOp* fop)
{
  Image *image = fop->seq.image;
  unsigned char image_palette[256][3];
  int x, y, c, r, g, b;
  int depth = (image->getPixelFormat() == IMAGE_RGB) ? 32 : 8;
  bool need_pal = (image->getPixelFormat() == IMAGE_INDEXED)? true: false;

  FileHandle f(fop->filename.c_str(), "wb");

  fputc(0, f);                          /* id length (no id saved) */
  fputc((need_pal) ? 1 : 0, f);         /* palette type */
  /* image type */
  fputc((image->getPixelFormat() == IMAGE_RGB      ) ? 2 :
        (image->getPixelFormat() == IMAGE_GRAYSCALE) ? 3 :
        (image->getPixelFormat() == IMAGE_INDEXED  ) ? 1 : 0, f);
  fputw(0, f);                         /* first colour */
  fputw((need_pal) ? 256 : 0, f);      /* number of colours */
  fputc((need_pal) ? 24 : 0, f);       /* palette entry size */
  fputw(0, f);                         /* left */
  fputw(0, f);                         /* top */
  fputw(image->w, f);                  /* width */
  fputw(image->h, f);                  /* height */
  fputc(depth, f);                     /* bits per pixel */

  /* descriptor (bottom to top, 8-bit alpha) */
  fputc(image->getPixelFormat() == IMAGE_RGB ? 8: 0, f);

  if (need_pal) {
    for (y=0; y<256; y++) {
      fop_sequence_get_color(fop, y, &r, &g, &b);
      image_palette[y][2] = r;
      image_palette[y][1] = g;
      image_palette[y][0] = b;
    }
    fwrite(image_palette, 1, 768, f);
  }

  switch (image->getPixelFormat()) {

    case IMAGE_RGB:
      for (y=image->h-1; y>=0; y--) {
        for (x=0; x<image->w; x++) {
          c = image_getpixel(image, x, y);
          fputc(_rgba_getb(c), f);
          fputc(_rgba_getg(c), f);
          fputc(_rgba_getr(c), f);
          fputc(_rgba_geta(c), f);
        }

        fop_progress(fop, (float)(image->h-y) / (float)(image->h));
      }
      break;

    case IMAGE_GRAYSCALE:
      for (y=image->h-1; y>=0; y--) {
        for (x=0; x<image->w; x++)
          fputc(_graya_getv(image_getpixel(image, x, y)), f);

        fop_progress(fop, (float)(image->h-y) / (float)(image->h));
      }
      break;

    case IMAGE_INDEXED:
      for (y=image->h-1; y>=0; y--) {
        for (x=0; x<image->w; x++)
          fputc(image_getpixel(image, x, y), f);

        fop_progress(fop, (float)(image->h-y) / (float)(image->h));
      }
      break;
  }

  if (ferror(f)) {
    fop_error(fop, "Error writing file.\n");
    return false;
  }
  else {
    return true;
  }
}
