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
 * ico.c - Based on the code of Elias Pschernig.
 */

#include "config.h"

#include <allegro/color.h>

#include "file/file.h"
#include "raster/raster.h"

/* static bool load_ICO(FileOp *fop); */
static bool save_ICO(FileOp *fop);

FileFormat format_ico =
{
  "ico",
  "ico",
  NULL, /* load_ICO, */
  save_ICO,
/*   FILE_SUPPORT_RGB | */
/*   FILE_SUPPORT_GRAY | */
  FILE_SUPPORT_INDEXED
};

/* static bool load_ICO(FileOp *fop) */
/* { */
/*   return NULL;			/\* TODO *\/ */
/* } */

static bool save_ICO(FileOp *fop)
{
  FILE *f;
  int depth, bpp, bw, bitsw;
  int size, offset, n, i;
  int c, x, y, b, m, v;
  int num = fop->sprite->frames;
  Image *bmp;

  f = fopen(fop->filename, "wb");
  if (!f)
    return FALSE;

  offset = 6 + num * 16;  /* ICONDIR + ICONDIRENTRYs */
   
  /* ICONDIR */
  fputw(0, f);			/* reserved            */
  fputw(1, f);			/* resource type: ICON */
  fputw(num, f);		/* number of icons     */

  for (n=0; n<num; ++n) {
    depth = 8;/* bitmap_color_depth(bmp[n]); */
    bpp = (depth == 8) ? 8 : 24;
    bw = (((fop->sprite->w * bpp / 8) + 3) / 4) * 4;
    bitsw = ((((fop->sprite->w + 7) / 8) + 3) / 4) * 4;
    size = fop->sprite->h * (bw + bitsw) + 40;

    if (bpp == 8)
      size += 256 * 4;

    /* ICONDIRENTRY */
    fputc(fop->sprite->w, f);	/* width                       */
    fputc(fop->sprite->h, f);	/* height                      */
    fputc(0, f);		/* color count                 */
    fputc(0, f);		/* reserved                    */
    fputw(1, f);		/* color planes                */
    fputw(bpp, f);		/* bits per pixel              */
    fputl(size, f);		/* size in bytes of image data */
    fputl(offset, f);		/* file offset to image data   */

    offset += size;
  }

  bmp = image_new(fop->sprite->imgtype,
		  fop->sprite->w,
		  fop->sprite->h);

  for (n=0; n<num; ++n) {
    image_clear(bmp, 0);
    layer_render(fop->sprite->set, bmp, 0, 0, n);

    depth = 8; /* bitmap_color_depth(bmp); */
    bpp = (depth == 8) ? 8 : 24;
    bw = (((bmp->w * bpp / 8) + 3) / 4) * 4;
    bitsw = ((((bmp->w + 7) / 8) + 3) / 4) * 4;
    size = bmp->h * (bw + bitsw) + 40;

    if (bpp == 8)
      size += 256 * 4;

    /* BITMAPINFOHEADER */
    fputl(40, f);		   /* size           */
    fputl(bmp->w, f);		   /* width          */
    fputl(bmp->h * 2, f);	   /* height x 2     */
    fputw(1, f);		   /* planes         */
    fputw(bpp, f);		   /* bitcount       */
    fputl(0, f);		   /* unused for ico */
    fputl(size, f);		   /* size           */
    fputl(0, f);		   /* unused for ico */
    fputl(0, f);		   /* unused for ico */
    fputl(0, f);		   /* unused for ico */
    fputl(0, f);		   /* unused for ico */

    /* PALETTE */
    if (bpp == 8) {
      Palette *pal = sprite_get_palette(fop->sprite, n);

      fputl(0, f);  /* color 0 is black, so the XOR mask works */

      for (i=1; i<256; i++) {
	fputc(_rgba_getb(pal->color[i]), f);
	fputc(_rgba_getg(pal->color[i]), f);
	fputc(_rgba_getr(pal->color[i]), f);
	fputc(0, f);
      }
    }

    /* XOR MASK */
    for (y=bmp->h-1; y>=0; --y) {
      for (x=0; x<bmp->w; ++x) {
	if (bpp == 8) {
	  fputc(image_getpixel(bmp, x, y), f);
	}
	else {
	  c = image_getpixel(bmp, x, y);
	  fputc(getb_depth(depth, c), f);
	  fputc(getg_depth(depth, c), f);
	  fputc(getr_depth(depth, c), f);
	}
      }

      /* every scanline must be 32-bit aligned */
      while (x & 3) {
	fputc(0, f);
	x++;
      } 
    }

    /* AND MASK */
    for (y=bmp->h-1; y>=0; --y) {
      for (x=0; x<(bmp->w+7)/8; ++x) {
	m = 0;
	v = 128;

	for (b=0; b<8; b++) {
	  c = image_getpixel(bmp, x*8+b, y);
	  if (c == 0/* bitmap_mask_color(bmp) */)
	    m += v;
	  v /= 2;
	}

	fputc(m, f);  
      }

      /* every scanline must be 32-bit aligned */
      while (x & 3) {
	fputc(0, f);
	x++;
      }
    }
  }

  image_free(bmp);
  fclose(f);

  return TRUE;
}
