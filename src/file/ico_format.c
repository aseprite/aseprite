/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro/color.h>
#include <allegro/file.h>

#include "console/console.h"
#include "file/file.h"
#include "raster/raster.h"

#endif

static Sprite *load_ICO(const char *filename);
static int save_ICO(Sprite *sprite);

FileFormat format_ico =
{
  "ico",
  "ico",
  load_ICO,
  save_ICO,
/*   FILE_SUPPORT_RGB | */
/*   FILE_SUPPORT_GRAY | */
  FILE_SUPPORT_INDEXED
};

static Sprite *load_ICO(const char *filename)
{
  return NULL;
}

static int save_ICO(Sprite *sprite)
{
  PACKFILE *f;
  int depth, bpp, bw, bitsw;
  int size, offset, n, i;
  int c, x, y, b, m, v;
  int num = sprite->frames;
  Image *bmp;

  errno = 0;

  f = pack_fopen(sprite->filename, F_WRITE);
  if (!f)
    return errno;

  offset = 6 + num * 16;  /* ICONDIR + ICONDIRENTRYs */
   
  /* ICONDIR */
  pack_iputw(0, f);    /* reserved            */
  pack_iputw(1, f);    /* resource type: ICON */
  pack_iputw(num, f);  /* number of icons     */

  for(n = 0; n < num; n++) {
    depth = 8;/* bitmap_color_depth(bmp[n]); */
    bpp = (depth == 8) ? 8 : 24;
    bw = (((sprite->w * bpp / 8) + 3) / 4) * 4;
    bitsw = ((((sprite->w + 7) / 8) + 3) / 4) * 4;
    size = sprite->h * (bw + bitsw) + 40;

    if (bpp == 8)
      size += 256 * 4;

    /* ICONDIRENTRY */
    pack_putc(sprite->w, f);  /* width                       */
    pack_putc(sprite->h, f);  /* height                      */
    pack_putc(0, f);          /* color count                 */
    pack_putc(0, f);          /* reserved                    */
    pack_iputw(1, f);         /* color planes                */
    pack_iputw(bpp, f);       /* bits per pixel              */
    pack_iputl(size, f);      /* size in bytes of image data */
    pack_iputl(offset, f);    /* file offset to image data   */

    offset += size;
  }

  bmp = image_new(sprite->imgtype, sprite->w, sprite->h);

  for(n = 0; n < num; n++) {
    image_clear(bmp, 0);
    layer_render(sprite->set, bmp, 0, 0, n);

    depth = 8;//bitmap_color_depth(bmp);
    bpp = (depth == 8) ? 8 : 24;
    bw = (((bmp->w * bpp / 8) + 3) / 4) * 4;
    bitsw = ((((bmp->w + 7) / 8) + 3) / 4) * 4;
    size = bmp->h * (bw + bitsw) + 40;

    if (bpp == 8)
      size += 256 * 4;

    /* BITMAPINFOHEADER */
    pack_iputl(40, f);             /* size           */
    pack_iputl(bmp->w, f);	   /* width          */
    pack_iputl(bmp->h * 2, f);	   /* height x 2     */
    pack_iputw(1, f);              /* planes         */
    pack_iputw(bpp, f);            /* bitcount       */
    pack_iputl(0, f);              /* unused for ico */
    pack_iputl(size, f);           /* size           */
    pack_iputl(0, f);              /* unused for ico */
    pack_iputl(0, f);              /* unused for ico */
    pack_iputl(0, f);              /* unused for ico */
    pack_iputl(0, f);              /* unused for ico */

    /* PALETTE */
    if (bpp == 8) {
      RGB *pal = sprite_get_palette(sprite, n);

      pack_iputl(0, f);  /* color 0 is black, so the XOR mask works */

      for (i = 1; i<256; i++) {
	pack_putc(_rgb_scale_6[pal[i].b], f);
	pack_putc(_rgb_scale_6[pal[i].g], f);
	pack_putc(_rgb_scale_6[pal[i].r], f);
	pack_putc(0, f);
      }
    }

    /* XOR MASK */
    for (y = bmp->h - 1; y >= 0; y--) {
      for (x = 0; x < bmp->w; x++) {
	if (bpp == 8) {
	  pack_putc(image_getpixel(bmp, x, y), f);
	}
	else {
	  c = image_getpixel(bmp, x, y);
	  pack_putc(getb_depth(depth, c), f);
	  pack_putc(getg_depth(depth, c), f);
	  pack_putc(getr_depth(depth, c), f);
	}
      }

      /* every scanline must be 32-bit aligned */
      while (x&3) {
	pack_putc(0, f);
	x++;
      } 
    }

    /* AND MASK */
    for (y = bmp->h - 1; y >= 0; y--) {
      for (x = 0; x < (bmp->w + 7)/8; x++) {
	m = 0;
	v = 128;

	for (b = 0; b < 8; b++) {
	  c = image_getpixel(bmp, x * 8 + b, y);
	  if (c == 0/* bitmap_mask_color(bmp) */)
	    m += v;
	  v /= 2;
	}

	pack_putc(m, f);  
      }

      /* every scanline must be 32-bit aligned */
      while (x&3) {
	pack_putc(0, f);
	x++;
      }
    }
  }

  image_free(bmp);
  pack_fclose(f);

  return errno;
}
