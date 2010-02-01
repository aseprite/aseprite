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
 *
 * pcx.c - Based on the code of Shawn Hargreaves.
 */

#include "config.h"

#include <allegro/color.h>

#include "file/file.h"
#include "raster/raster.h"

static bool load_PCX(FileOp *fop);
static bool save_PCX(FileOp *fop);

FileFormat format_pcx =
{
  "pcx",
  "pcx",
  load_PCX,
  save_PCX,
  NULL,
  FILE_SUPPORT_RGB |
  FILE_SUPPORT_GRAY |
  FILE_SUPPORT_INDEXED |
  FILE_SUPPORT_SEQUENCES
};

static bool load_PCX(FileOp *fop)
{
  Image *image;
  FILE *f;
  int c, r, g, b;
  int width, height;
  int bpp, bytes_per_line;
  int xx, po;
  int x, y;
  char ch = 0;

  f = fopen(fop->filename, "rb");
  if (!f)
    return false;

  fgetc(f);                    /* skip manufacturer ID */
  fgetc(f);                    /* skip version flag */
  fgetc(f);                    /* skip encoding flag */

  if (fgetc(f) != 8) {         /* we like 8 bit color planes */
    fop_error(fop, _("This PCX doesn't have 8 bit color planes.\n"));
    fclose(f);
    return false;
  }

  width = -(fgetw(f));		/* xmin */
  height = -(fgetw(f));		/* ymin */
  width += fgetw(f) + 1;	/* xmax */
  height += fgetw(f) + 1;	/* ymax */

  fgetl(f);			/* skip DPI values */

  for (c=0; c<16; c++) {	/* read the 16 color palette */
    r = fgetc(f);
    g = fgetc(f);
    b = fgetc(f);
    fop_sequence_set_color(fop, c, r, g, b);
  }

  fgetc(f);

  bpp = fgetc(f) * 8;          /* how many color planes? */
  if ((bpp != 8) && (bpp != 24)) {
    fclose(f);
    return false;
  }

  bytes_per_line = fgetw(f);

  for (c=0; c<60; c++)             /* skip some more junk */
    fgetc(f);

  image = fop_sequence_image(fop, bpp == 8 ?
				  IMAGE_INDEXED:
				  IMAGE_RGB,
			     width, height);
  if (!image) {
    fclose(f);
    return false;
  }

  if (bpp == 24)
    image_clear(image, _rgba(0, 0, 0, 255));

  for (y=0; y<height; y++) {       /* read RLE encoded PCX data */
    x = xx = 0;
    po = _rgba_r_shift;

    while (x < bytes_per_line*bpp/8) {
      ch = fgetc(f);
      if ((ch & 0xC0) == 0xC0) {
        c = (ch & 0x3F);
        ch = fgetc(f);
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
    if (fop_is_stop(fop))
      break;
  }

  if (!fop_is_stop(fop)) {
    if (bpp == 8) {                  /* look for a 256 color palette */
      while ((c = fgetc(f)) != EOF) {
	if (c == 12) {
	  for (c=0; c<256; c++) {
	    r = fgetc(f);
	    g = fgetc(f);
	    b = fgetc(f);
	    fop_sequence_set_color(fop, c, r, g, b);
	  }
	  break;
	}
      }
    }
  }

  if (ferror(f)) {
    fop_error(fop, _("Error reading file.\n"));
    fclose(f);
    return false;
  }
  else {
    fclose(f);
    return true;
  }
}

static bool save_PCX(FileOp *fop)
{
  Image *image = fop->seq.image;
  FILE *f;
  int c, r, g, b;
  int x, y;
  int runcount;
  int depth, planes;
  char runchar;
  char ch = 0;

  f = fopen(fop->filename, "wb");
  if (!f) {
    fop_error(fop, _("Error creating file.\n"));
    return false;
  }

  if (image->imgtype == IMAGE_RGB) {
    depth = 24;
    planes = 3;
  }
  else {
    depth = 8;
    planes = 1;
  }

  fputc(10, f);                      /* manufacturer */
  fputc(5, f);                       /* version */
  fputc(1, f);                       /* run length encoding  */
  fputc(8, f);                       /* 8 bits per pixel */
  fputw(0, f);			     /* xmin */
  fputw(0, f);			     /* ymin */
  fputw(image->w-1, f);		     /* xmax */
  fputw(image->h-1, f);		     /* ymax */
  fputw(320, f);		     /* HDpi */
  fputw(200, f);		     /* VDpi */

  for (c=0; c<16; c++) {
    fop_sequence_get_color(fop, c, &r, &g, &b);
    fputc(r, f);
    fputc(g, f);
    fputc(b, f);
  }

  fputc(0, f);                      /* reserved */
  fputc(planes, f);                 /* one or three color planes */
  fputw(image->w, f);               /* number of bytes per scanline */
  fputw(1, f);                      /* color palette */
  fputw(image->w, f);               /* hscreen size */
  fputw(image->h, f);               /* vscreen size */
  for (c=0; c<54; c++)		    /* filler */
    fputc(0, f);

  for (y=0; y<image->h; y++) {           /* for each scanline... */
    runcount = 0;
    runchar = 0;
    for (x=0; x<image->w*planes; x++) {  /* for each pixel... */
      if (depth == 8) {
	if (image->imgtype == IMAGE_INDEXED)
	  ch = image_getpixel_fast<IndexedTraits>(image, x, y);
	else if (image->imgtype == IMAGE_GRAYSCALE) {
	  c = image_getpixel_fast<GrayscaleTraits>(image, x, y);
	  ch = _graya_getv(c);
	}
      }
      else {
        if (x < image->w) {
          c = image_getpixel_fast<RgbTraits>(image, x, y);
          ch = _rgba_getr(c);
        }
        else if (x<image->w*2) {
          c = image_getpixel_fast<RgbTraits>(image, x-image->w, y);
          ch = _rgba_getg(c);
        }
        else {
          c = image_getpixel_fast<RgbTraits>(image, x-image->w*2, y);
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
            fputc(0xC0 | runcount, f);
          fputc(runchar, f);
          runcount = 1;
          runchar = ch;
        }
        else
          runcount++;
      }
    }

    if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
      fputc(0xC0 | runcount, f);

    fputc(runchar, f);

    fop_progress(fop, (float)(y+1) / (float)(image->h));
  }

  if (depth == 8) {                      /* 256 color palette */
    fputc(12, f);

    for (c=0; c<256; c++) {
      fop_sequence_get_color(fop, c, &r, &g, &b);
      fputc(r, f);
      fputc(g, f);
      fputc(b, f);
    }
  }

  if (ferror(f)) {
    fop_error(fop, _("Error writing file.\n"));
    fclose(f);
    return false;
  }
  else {
    fclose(f);
    return true;
  }
}
