/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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
 * pcx.c - Based on the code of Shawn Hargreaves.
 */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro/color.h>
#include <allegro/file.h>

#include "file/file.h"
#include "raster/raster.h"

#endif

static bool load_PCX(FileOp *fop);
static bool save_PCX(FileOp *fop);

FileFormat format_pcx =
{
  "pcx",
  "pcx",
  load_PCX,
  save_PCX,
  FILE_SUPPORT_RGB |
  FILE_SUPPORT_GRAY |
  FILE_SUPPORT_INDEXED |
  FILE_SUPPORT_SEQUENCES
};

static bool load_PCX(FileOp *fop)
{
  Image *image;
  PACKFILE *f;
  int c, r, g, b;
  int width, height;
  int bpp, bytes_per_line;
  int xx, po;
  int x, y;
  char ch = 0;

  f = pack_fopen(fop->filename, F_READ);
  if (!f)
    return FALSE;

  pack_getc(f);                    /* skip manufacturer ID */
  pack_getc(f);                    /* skip version flag */
  pack_getc(f);                    /* skip encoding flag */

  if (pack_getc(f) != 8) {         /* we like 8 bit color planes */
    fop_error(fop, _("This PCX doesn't have 8 bit color planes.\n"));
    pack_fclose(f);
    return FALSE;
  }

  width = -(pack_igetw(f));        /* xmin */
  height = -(pack_igetw(f));       /* ymin */
  width += pack_igetw(f) + 1;      /* xmax */
  height += pack_igetw(f) + 1;     /* ymax */

  pack_igetl(f);                   /* skip DPI values */

  for (c=0; c<16; c++) {           /* read the 16 color palette */
    r = pack_getc(f) / 4;
    g = pack_getc(f) / 4;
    b = pack_getc(f) / 4;
    fop_sequence_set_color(fop, c, r, g, b);
  }

  pack_getc(f);

  bpp = pack_getc(f) * 8;          /* how many color planes? */
  if ((bpp != 8) && (bpp != 24)) {
    pack_fclose(f);
    return FALSE;
  }

  bytes_per_line = pack_igetw(f);

  for (c=0; c<60; c++)             /* skip some more junk */
    pack_getc(f);

  image = fop_sequence_image(fop, bpp == 8 ?
				  IMAGE_INDEXED:
				  IMAGE_RGB,
			     width, height);
  if (!image) {
    pack_fclose(f);
    return FALSE;
  }

  if (bpp == 24)
    image_clear(image, _rgba(0, 0, 0, 255));

  *allegro_errno = 0;

  for (y=0; y<height; y++) {       /* read RLE encoded PCX data */
    x = xx = 0;
    po = _rgba_r_shift;

    while (x < bytes_per_line*bpp/8) {
      ch = pack_getc(f);
      if ((ch & 0xC0) == 0xC0) {
        c = (ch & 0x3F);
        ch = pack_getc(f);
      }
      else
        c = 1;

      if (bpp == 8) {
        while (c--) {
          if (x < image->w)
            *(((ase_uint8 **)image->line)[y]+x) = ch;
          x++;
        }
      }
      else {
        while (c--) {
          if (xx < image->w)
            *(((ase_uint32 **)image->line)[y]+xx) |= (ch & 0xff) << po;
          x++;
          if (x == bytes_per_line) {
            xx = 0;
            po = _rgba_g_shift;
          }
          else if (x == bytes_per_line*2) {
            xx = 0;
            po = _rgba_b_shift;
          }
          else
            xx++;
        }
      }
    }

    fop_progress(fop, (float)(y+1) / (float)(height));
  }

  if (bpp == 8) {                  /* look for a 256 color palette */
    while ((c = pack_getc(f)) != EOF) {
      if (c == 12) {
        for (c=0; c<256; c++) {
          r = pack_getc(f) / 4;
          g = pack_getc(f) / 4;
          b = pack_getc(f) / 4;
          fop_sequence_set_color(fop, c, r, g, b);
        }
        break;
      }
    }
  }

  if (*allegro_errno) {
    fop_error(fop, _("Error reading bytes.\n"));
    pack_fclose(f);
    return FALSE;
  }

  pack_fclose(f);
  return TRUE;
}

static bool save_PCX(FileOp *fop)
{
  Image *image = fop->seq.image;
  PACKFILE *f;
  int c, r, g, b;
  int x, y;
  int runcount;
  int depth, planes;
  char runchar;
  char ch = 0;

  f = pack_fopen(fop->filename, F_WRITE);
  if (!f) {
    fop_error(fop, _("Error creating file.\n"));
    return FALSE;
  }

  if (image->imgtype == IMAGE_RGB) {
    depth = 24;
    planes = 3;
  }
  else {
    depth = 8;
    planes = 1;
  }

  *allegro_errno = 0;

  pack_putc(10, f);                      /* manufacturer */
  pack_putc(5, f);                       /* version */
  pack_putc(1, f);                       /* run length encoding  */
  pack_putc(8, f);                       /* 8 bits per pixel */
  pack_iputw(0, f);                      /* xmin */
  pack_iputw(0, f);                      /* ymin */
  pack_iputw(image->w-1, f);             /* xmax */
  pack_iputw(image->h-1, f);             /* ymax */
  pack_iputw(320, f);                    /* HDpi */
  pack_iputw(200, f);                    /* VDpi */

  for (c=0; c<16; c++) {
    fop_sequence_get_color(fop, c, &r, &g, &b);
    pack_putc(_rgb_scale_6[r], f);
    pack_putc(_rgb_scale_6[g], f);
    pack_putc(_rgb_scale_6[b], f);
  }

  pack_putc(0, f);                       /* reserved */
  pack_putc(planes, f);                  /* one or three color planes */
  pack_iputw(image->w, f);               /* number of bytes per scanline */
  pack_iputw(1, f);                      /* color palette */
  pack_iputw(image->w, f);               /* hscreen size */
  pack_iputw(image->h, f);               /* vscreen size */
  for (c=0; c<54; c++)                   /* filler */
    pack_putc(0, f);

  for (y=0; y<image->h; y++) {           /* for each scanline... */
    runcount = 0;
    runchar = 0;
    for (x=0; x<image->w*planes; x++) {  /* for each pixel... */
      if (depth == 8) {
	if (image->imgtype == IMAGE_INDEXED)
	  ch = image->method->getpixel(image, x, y);
	else if (image->imgtype == IMAGE_GRAYSCALE) {
	  c = image->method->getpixel(image, x, y);
	  ch = _graya_getk(c);
	}
      }
      else {
        if (x < image->w) {
          c = image->method->getpixel(image, x, y);
          ch = _rgba_getr(c);
        }
        else if (x<image->w*2) {
          c = image->method->getpixel(image, x-image->w, y);
          ch = _rgba_getg(c);
        }
        else {
          c = image->method->getpixel(image, x-image->w*2, y);
          ch = _rgba_getb(c);
        }
      }
      if (runcount == 0) {
        runcount = 1;
        runchar = ch;
      }
      else {
        if ((ch != runchar) || (runcount >= 0x3f)) {
          if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
            pack_putc(0xC0 | runcount, f);
          pack_putc(runchar, f);
          runcount = 1;
          runchar = ch;
        }
        else
          runcount++;
      }
    }

    if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
      pack_putc(0xC0 | runcount, f);

    pack_putc(runchar, f);

    fop_progress(fop, (float)(y+1) / (float)(image->h));
  }

  if (depth == 8) {                      /* 256 color palette */
    pack_putc(12, f);

    for (c=0; c<256; c++) {
      fop_sequence_get_color(fop, c, &r, &g, &b);
      pack_putc(_rgb_scale_6[r], f);
      pack_putc(_rgb_scale_6[g], f);
      pack_putc(_rgb_scale_6[b], f);
    }
  }

  pack_fclose(f);

  if (*allegro_errno) {
    fop_error(fop, _("Error writing bytes.\n"));
    return FALSE;
  }
  else
    return TRUE;
}
