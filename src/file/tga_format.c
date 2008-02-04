/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007, 2008  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro/color.h>
#include <allegro/file.h>

#include "jinete/jbase.h"

#include "file/file.h"
#include "raster/raster.h"

#endif

static bool load_TGA(FileOp *fop);
static bool save_TGA(FileOp *fop);

FileFormat format_tga =
{
  "tga",
  "tga",
  load_TGA,
  save_TGA,
  FILE_SUPPORT_RGB |
  FILE_SUPPORT_RGBA |
  FILE_SUPPORT_GRAY |
  FILE_SUPPORT_INDEXED |
  FILE_SUPPORT_SEQUENCES
};

/* rle_tga_read:
 *  Helper for reading 256 color RLE data from TGA files.
 */
static void rle_tga_read(unsigned char *address, int w, int type, PACKFILE *f)
{
  unsigned char value;
  int count, g;
  int c = 0;

  do {
    count = pack_getc(f);
    if (count & 0x80) {
      count = (count & 0x7F) + 1;
      c += count;
      value = pack_getc(f);
      while (count--) {
	if (type == 1)
	  *(address++) = value;
	else {
	  *((ase_uint16 *)address) = value;
	  address += sizeof(ase_uint16);
	}
      }
    }
    else {
      count++;
      c += count;
      if (type == 1) {
	pack_fread(address, count, f);
	address += count;
      }
      else {
	for (g=0; g<count; g++) {
	  *((ase_uint16 *)address) = pack_getc(f);
	  address += sizeof(ase_uint16);
	}
      }
    }
  } while (c < w);
}

/* rle_tga_read32:
 *  Helper for reading 32 bit RLE data from TGA files.
 */
static void rle_tga_read32 (ase_uint32 *address, int w, PACKFILE *f)
{
  unsigned char value[4];
  int count;
  int c = 0;

  do {
    count = pack_getc(f);
    if (count & 0x80) {
      count = (count & 0x7F) + 1;
      c += count;
      pack_fread(value, 4, f);
      while (count--)
        *(address++) = _rgba(value[2], value[1], value[0], value[3]);
    }
    else {
      count++;
      c += count;
      while (count--) {
        pack_fread(value, 4, f);
        *(address++) = _rgba(value[2], value[1], value[0], value[3]);
      }
    }
  } while (c < w);
}

/* rle_tga_read24:
 *  Helper for reading 24 bit RLE data from TGA files.
 */
static void rle_tga_read24(ase_uint32 *address, int w, PACKFILE *f)
{
  unsigned char value[4];
  int count;
  int c = 0;

  do {
    count = pack_getc(f);
    if (count & 0x80) {
      count = (count & 0x7F) + 1;
      c += count;
      pack_fread(value, 3, f);
      while (count--)
        *(address++) = _rgba(value[2], value[1], value[0], 255);
    }
    else {
      count++;
      c += count;
      while (count--) {
        pack_fread(value, 3, f);
        *(address++) = _rgba(value[2], value[1], value[0], 255);
      }
    }
  } while (c < w);
}

/* rle_tga_read16:
 *  Helper for reading 16 bit RLE data from TGA files.
 */
static void rle_tga_read16(ase_uint32 *address, int w, PACKFILE *f)
{
  unsigned int value;
  ase_uint32 color;
  int count;
  int c = 0;

  do {
    count = pack_getc(f);
    if (count & 0x80) {
      count = (count & 0x7F) + 1;
      c += count;
      value = pack_igetw(f);
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
        value = pack_igetw(f);
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
static bool load_TGA(FileOp *fop)
{
  unsigned char image_id[256], image_palette[256][3], rgb[4];
  unsigned char id_length, palette_type, image_type, palette_entry_size;
  unsigned char bpp, descriptor_bits;
  short unsigned int first_color, palette_colors;
  short unsigned int left, top, image_width, image_height;
  unsigned int c, i, x, y, yc;
  Image *image;
  int compressed;
  PACKFILE *f;
  int type;

  f = pack_fopen(fop->filename, F_READ);
  if (!f)
    return FALSE;

  *allegro_errno = 0;

  id_length = pack_getc(f);
  palette_type = pack_getc(f);
  image_type = pack_getc(f);
  first_color = pack_igetw(f);
  palette_colors  = pack_igetw(f);
  palette_entry_size = pack_getc(f);
  left = pack_igetw(f);
  top = pack_igetw(f);
  image_width = pack_igetw(f);
  image_height = pack_igetw(f);
  bpp = pack_getc(f);
  descriptor_bits = pack_getc(f);

  pack_fread(image_id, id_length, f);

  if (palette_type == 1) {
    for (i=0; i<palette_colors; i++) {
      switch (palette_entry_size) {

	case 16:
	  c = pack_igetw(f);
	  image_palette[i][0] = (c & 0x1F) << 3;
	  image_palette[i][1] = ((c >> 5) & 0x1F) << 3;
	  image_palette[i][2] = ((c >> 10) & 0x1F) << 3;
	  break;

	case 24:
	case 32:
	  image_palette[i][0] = pack_getc(f);
	  image_palette[i][1] = pack_getc(f);
	  image_palette[i][2] = pack_getc(f);
	  if (palette_entry_size == 32)
	    pack_getc(f);
	  break;
      }
    }
  }
  else if (palette_type != 0) {
    pack_fclose(f);
    return FALSE;
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

  switch (image_type) {

    /* paletted image */
    case 1:
      if ((palette_type != 1) || (bpp != 8)) {
        pack_fclose(f);
        return FALSE;
      }

      for (i=0; i<palette_colors; i++) {
        fop_sequence_set_color(fop, i,
			       image_palette[i][2] >> 2,
			       image_palette[i][1] >> 2,
			       image_palette[i][0] >> 2);
      }

      type = IMAGE_INDEXED;
      break;

    /* truecolor image */
    case 2:
      if ((palette_type != 0) ||
          ((bpp != 15) && (bpp != 16) &&
           (bpp != 24) && (bpp != 32))) {
        pack_fclose(f);
        return FALSE;
      }

      type = IMAGE_RGB;
      break;

    /* grayscale image */
    case 3:
      if ((palette_type != 0) || (bpp != 8)) {
        pack_fclose(f);
        return FALSE;
      }

      for (i=0; i<256; i++)
        fop_sequence_set_color(fop, i, i>>2, i>>2, i>>2);

      type = IMAGE_GRAYSCALE;
      break;

    default:
      /* TODO add support for more TGA types? */

      pack_fclose(f);
      return FALSE;
  }

  image = fop_sequence_image(fop, type, image_width, image_height);
  if (!image) {
    pack_fclose(f);
    return FALSE;
  }

  for (y=image_height; y; y--) {
    yc = (descriptor_bits & 0x20) ? image_height-y : y-1;

    switch (image_type) {

      case 1:
      case 3:
        if (compressed)
          rle_tga_read(image->line[yc], image_width, image_type, f);
        else if (image_type == 1)
          pack_fread(image->line[yc], image_width, f);
	else {
	  for (x=0; x<image_width; x++)
	    *(((ase_uint16 **)image->line)[yc]+x) =
	      _graya(pack_getc(f), 255);
	}
	break;

      case 2:
        if (bpp == 32) {
          if (compressed) {
            rle_tga_read32((ase_uint32 *)image->line[yc], image_width, f);
          }
          else {
            for (x=0; x<image_width; x++) {
              pack_fread(rgb, 4, f);
              *(((ase_uint32 **)image->line)[yc]+x) =
                _rgba(rgb[2], rgb[1], rgb[0], rgb[3]);
            }
          }
        }
        else if (bpp == 24) {
          if (compressed) {
            rle_tga_read24((ase_uint32 *)image->line[yc], image_width, f);
          }
          else {
            for (x=0; x<image_width; x++) {
              pack_fread(rgb, 3, f);
              *(((ase_uint32 **)image->line)[yc]+x) =
                _rgba(rgb[2], rgb[1], rgb[0], 255);
            }
          }
        }
        else {
          if (compressed) {
            rle_tga_read16((ase_uint32 *)image->line[yc], image_width, f);
          }
          else {
            for (x=0; x<image_width; x++) {
              c = pack_igetw(f);
              *(((ase_uint32 **)image->line)[yc]+x) =
                _rgba(((c >> 10) & 0x1F),
		      ((c >> 5) & 0x1F),
		      (c & 0x1F), 255);
            }
          }
        }
        break;
    }

    if (image_height > 1)
      fop_progress(fop, (float)(image_height-y) / (float)(image_height));
  }

  if (*allegro_errno) {
    fop_error(fop, _("Error reading bytes.\n"));
    pack_fclose(f);
    return FALSE;
  }

  pack_fclose(f);
  return TRUE;
}

/* save_tga:
 *  Writes a bitmap into a TGA file, using the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
static bool save_TGA(FileOp *fop)
{
  Image *image = fop->seq.image;
  unsigned char image_palette[256][3];
  int x, y, c, r, g, b;
  int depth = (image->imgtype == IMAGE_RGB) ? 32 : 8;
  bool need_pal = (image->imgtype == IMAGE_INDEXED)? TRUE: FALSE;
  PACKFILE *f;

  f = pack_fopen(fop->filename, F_WRITE);
  if (!f) {
    fop_error(fop, _("Error creating file.\n"));
    return FALSE;
  }

  *allegro_errno = 0;

  pack_putc(0, f);                          /* id length (no id saved) */
  pack_putc((need_pal) ? 1 : 0, f);         /* palette type */
  /* image type */
  pack_putc((image->imgtype == IMAGE_RGB      ) ? 2 :
            (image->imgtype == IMAGE_GRAYSCALE) ? 3 :
            (image->imgtype == IMAGE_INDEXED  ) ? 1 : 0, f);
  pack_iputw(0, f);                         /* first colour */
  pack_iputw((need_pal) ? 256 : 0, f);      /* number of colours */
  pack_putc((need_pal) ? 24 : 0, f);        /* palette entry size */
  pack_iputw(0, f);			    /* left */
  pack_iputw(0, f);			    /* top */
  pack_iputw(image->w, f);		    /* width */
  pack_iputw(image->h, f);		    /* height */
  pack_putc(depth, f);			    /* bits per pixel */

  /* descriptor (bottom to top, 8-bit alpha) */
  pack_putc(image->imgtype == IMAGE_RGB ? 8: 0, f);

  if (need_pal) {
    for (y=0; y<256; y++) {
      fop_sequence_get_color(fop, y, &r, &g, &b);
      image_palette[y][2] = _rgb_scale_6[r];
      image_palette[y][1] = _rgb_scale_6[g];
      image_palette[y][0] = _rgb_scale_6[b];
    }
    pack_fwrite(image_palette, 768, f);
  }

  switch (image->imgtype) {

    case IMAGE_RGB:
      for (y=image->h-1; y>=0; y--) {
        for (x=0; x<image->w; x++) {
          c = image_getpixel(image, x, y);
          pack_putc(_rgba_getb(c), f);
          pack_putc(_rgba_getg(c), f);
          pack_putc(_rgba_getr(c), f);
          pack_putc(_rgba_geta(c), f);
        }

	fop_progress(fop, (float)(image->h-y) / (float)(image->h));
      }
      break;

    case IMAGE_GRAYSCALE:
      for (y=image->h-1; y>=0; y--) {
        for (x=0; x<image->w; x++)
          pack_putc(_graya_getk(image_getpixel(image, x, y)), f);

	fop_progress(fop, (float)(image->h-y) / (float)(image->h));
      }
      break;

    case IMAGE_INDEXED:
      for (y=image->h-1; y>=0; y--) {
        for (x=0; x<image->w; x++)
          pack_putc(image_getpixel(image, x, y), f);

	fop_progress(fop, (float)(image->h-y) / (float)(image->h));
      }
      break;
  }

  pack_fclose(f);

  if (*allegro_errno) {
    fop_error(fop, _("Error writing bytes.\n"));
    return FALSE;
  }
  else
    return TRUE;
}
