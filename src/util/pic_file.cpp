/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "config.h"

#include <allegro/color.h>
#include <allegro/file.h>

#include "raster/image.h"

/* loads a PIC file (Animator and Animator Pro format) */
Image *load_pic_file(const char *filename, int *x, int *y, RGB *palette)
{
  int size, compression;
  int image_size;
  int block_size;
  int block_type;
  Image *image;
  PACKFILE *f;
  int version;
  int r, g, b;
  int c, u, v;
  int magic;
  int w, h;
  int byte;
  int bpp;

  f = pack_fopen (filename, F_READ);
  if (!f)
    return NULL;

  /* Animator format? */
  magic = pack_igetw (f);
  if (magic == 0x9119) {
    /* read Animator PIC file ***************************************/
    w = pack_igetw (f);			/* width */
    h = pack_igetw (f);			/* height */
    *x = ((short)pack_igetw (f));	/* X offset */
    *y = ((short)pack_igetw (f));	/* Y offset */
    bpp = pack_getc (f);		/* bits per pixel (must be 8) */
    compression = pack_getc (f);	/* compression flag (must be 0) */
    image_size = pack_igetl (f);	/* image size (in bytes) */
    pack_getc (f);			/* reserved */

    if (bpp != 8 || compression != 0) {
      pack_fclose (f);
      return NULL;
    }

    /* read palette (RGB in 0-63) */
    for (c=0; c<256; c++) {
      r = pack_getc (f);
      g = pack_getc (f);
      b = pack_getc (f);
      if (palette) {
	palette[c].r = r;
	palette[c].g = g;
	palette[c].b = b;
      }
    }

    /* read image */
    image = image_new(IMAGE_INDEXED, w, h);

    for (v=0; v<h; v++)
      for (u=0; u<w; u++)
	image->putpixel(u, v, pack_getc(f));

    pack_fclose (f);
    return image;
  }

  /* rewind */
  pack_fclose (f);
  f = pack_fopen (filename, F_READ);
  if (!f)
    return NULL;

  /* read a PIC/MSK Animator Pro file *************************************/
  size = pack_igetl (f);	/* file size */
  magic = pack_igetw (f);	/* magic number 9500h */
  if (magic != 0x9500) {
    pack_fclose (f);
    return NULL;
  }

  w = pack_igetw (f);		/* width */
  h = pack_igetw (f);		/* height */
  *x = pack_igetw (f);		/* X offset */
  *y = pack_igetw (f);		/* Y offset */
  pack_igetl (f);		/* user ID, is 0 */
  bpp = pack_getc (f);		/* bits per pixel */

  if ((bpp != 1 && bpp != 8) || (w<1) || (h<1) || (w>9999) || (h>9999)) {
    pack_fclose (f);
    return NULL;
  }

  /* skip reserved data */
  for (c=0; c<45; c++)
    pack_getc (f);

  size -= 64;			/* the header uses 64 bytes */

  image = image_new (bpp == 8 ? IMAGE_INDEXED: IMAGE_BITMAP, w, h);

  /* read blocks to end of file */
  while (size > 0) {
    block_size = pack_igetl (f);
    block_type = pack_igetw (f);

    switch (block_type) {

      /* color palette info */
      case 0:
	version = pack_igetw (f);	/* palette version */
	if (version != 0) {
	  image_free (image);
	  pack_fclose (f);
	  return NULL;
	}
	/* 256 RGB entries in 0-255 format */
	for (c=0; c<256; c++) {
	  r = pack_getc (f);
	  g = pack_getc (f);
	  b = pack_getc (f);
	  if (palette) {
	    palette[c].r = MID (0, r, 255) >> 2;
	    palette[c].g = MID (0, g, 255) >> 2;
	    palette[c].b = MID (0, b, 255) >> 2;
	  }
	}
	break;

      /* byte-per-pixel image data */
      case 1:
	for (v=0; v<h; v++)
	  for (u=0; u<w; u++)
	    image->putpixel(u, v, pack_getc(f));
	break;

      /* bit-per-pixel image data */
      case 2:
	for (v=0; v<h; v++)
	  for (u=0; u<(w+7)/8; u++) {
	    byte = pack_getc (f);
	    for (c=0; c<8; c++)
	      image_putpixel (image, u*8+c, v, byte & (1<<(7-c)));
	  }
	break;
    }

    size -= block_size;
  }

  pack_fclose (f);
  return image;
}

/* saves an Animator Pro PIC file */
int save_pic_file(const char *filename, int x, int y,
		  const RGB* palette, const Image* image)
{
  int c, u, v, bpp, size, byte;
  PACKFILE* f;

  if (image->imgtype == IMAGE_INDEXED)
    bpp = 8;
  else if (image->imgtype == IMAGE_BITMAP)
    bpp = 1;
  else
    return -1;

  if ((bpp == 8) && (!palette))
    return -1;

  f = pack_fopen (filename, F_WRITE);
  if (!f)
    return -1;

  size = 64;
  /* bit-per-pixel image data block */
  if (bpp == 1)
    size += (4+2+((image->w+7)/8)*image->h);
  /* color palette info + byte-per-pixel image data block */
  else
    size += (4+2+2+256*3) + (4+2+image->w*image->h);

  pack_iputl(size, f);		/* file size */
  pack_iputw(0x9500, f);	/* magic number 9500h */
  pack_iputw(image->w, f);	/* width */
  pack_iputw(image->h, f);	/* height */
  pack_iputw(x, f);		/* X offset */
  pack_iputw(y, f);		/* Y offset */
  pack_iputl(0, f);		/* user ID, is 0 */
  pack_putc(bpp, f);		/* bits per pixel */

  /* reserved data */
  for (c=0; c<45; c++)
    pack_putc(0, f);

  /* 1 bpp */
  if (bpp == 1) {
    /* bit-per-data image data block */
    pack_iputl ((4+2+((image->w+7)/8)*image->h), f);	/* block size */
    pack_iputw (2, f);					/* block type */
    for (v=0; v<image->h; v++)				/* image data */
      for (u=0; u<(image->w+7)/8; u++) {
	byte = 0;
	for (c=0; c<8; c++)
	  if (image_getpixel (image, u*8+c, v))
	    byte |= (1<<(7-c));
	pack_putc (byte, f);
      }
  }
  /* 8 bpp */
  else {
    /* color palette info */
    pack_iputl((4+2+2+256*3), f);	/* block size */
    pack_iputw(0, f);			/* block type */
    pack_iputw(0, f);			/* version */
    for (c=0; c<256; c++) {		/* 256 palette entries */
      pack_putc(_rgb_scale_6[palette[c].r], f);
      pack_putc(_rgb_scale_6[palette[c].g], f);
      pack_putc(_rgb_scale_6[palette[c].b], f);
    }

    /* pixel-per-data image data block */
    pack_iputl ((4+2+image->w*image->h), f);	/* block size */
    pack_iputw (1, f);				/* block type */
    for (v=0; v<image->h; v++)			/* image data */
      for (u=0; u<image->w; u++)
	pack_putc(image->getpixel(u, v), f);
  }

  pack_fclose (f);
  return 0;
}
