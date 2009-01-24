/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#undef BYTES
#undef LINES

#define BYTES(image)   ((ase_uint8 *)image->dat)
#define LINES(image)   ((ase_uint8 **)image->line)

static int alleg_init(Image *image)
{
  static int _image_depth[] = { 32, 16, 8, 8 };
  image->bmp = create_bitmap_ex(_image_depth[image->imgtype],
				image->w, image->h);
  image->dat = image->bmp->dat;
  image->line = image->bmp->line;
  return 0;
}

static int alleg_getpixel(const Image *image, int x, int y)
{
  return getpixel(image->bmp, x, y);
}

static void alleg_putpixel(Image *image, int x, int y, int color)
{
  putpixel(image->bmp, x, y, color);
}

static void alleg_clear(Image *image, int color)
{
  clear_to_color(image->bmp, color);
}

static void alleg_copy(Image *dst, const Image *src, int x, int y)
{
  blit(src->bmp, dst->bmp, 0, 0, x, y, src->w, src->h);
#if 0
  ase_uint8 *src_address;
  ase_uint8 *dst_address;
  int xbeg, xend, xsrc;
  int ybeg, yend, ysrc, ydst;
  int bytes;

  /* clipping */

  xsrc = 0;
  ysrc = 0;

  xbeg = x;
  ybeg = y;
  xend = x+src->w-1;
  yend = y+src->h-1;

  if ((xend < 0) || (xbeg >= dst->w) ||
      (yend < 0) || (ybeg >= dst->h))
    return;

  if (xbeg < 0) {
    xsrc -= xbeg;
    xbeg = 0;
  }

  if (ybeg < 0) {
    ysrc -= ybeg;
    ybeg = 0;
  }

  if (xend >= dst->w)
    xend = dst->w-1;

  if (yend >= dst->h)
    yend = dst->h-1;

  /* copy process */

  bytes = (xend - xbeg + 1);

  for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
    src_address = LINES (src)[ysrc]+xsrc;
    dst_address = LINES (dst)[ydst]+xbeg;

    memcpy (dst_address, src_address, bytes);
  }
#endif
}

/* if "color_map" is not NULL, it's used by the routine to merge the
   source and the destionation pixels */
static void alleg_merge(Image *dst, const Image *src,
			int x, int y, int opacity, int blend_mode)
{
  masked_blit(src->bmp, dst->bmp, 0, 0, x, y, src->w, src->h);
#if 0
  ase_uint8 *src_address;
  ase_uint8 *dst_address;
  int xbeg, xend, xsrc, xdst;
  int ybeg, yend, ysrc, ydst;

  /* clipping */

  xsrc = 0;
  ysrc = 0;

  xbeg = x;
  ybeg = y;
  xend = x+src->w-1;
  yend = y+src->h-1;

  if ((xend < 0) || (xbeg >= dst->w) ||
      (yend < 0) || (ybeg >= dst->h))
    return;

  if (xbeg < 0) {
    xsrc -= xbeg;
    xbeg = 0;
  }

  if (ybeg < 0) {
    ysrc -= ybeg;
    ybeg = 0;
  }

  if (xend >= dst->w)
    xend = dst->w-1;

  if (yend >= dst->h)
    yend = dst->h-1;

  /* merge process */

  /* direct copy */
  if (blend_mode == BLEND_MODE_COPY) {
    for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
      src_address = LINES (src)[ysrc]+xsrc;
      dst_address = LINES (dst)[ydst]+xbeg;

      for (xdst=xbeg; xdst<=xend; xdst++) {
	*dst_address = (*src_address);

	dst_address++;
	src_address++;
      }
    }
  }
  /* with mask */
  else {
    for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
      src_address = LINES (src)[ysrc]+xsrc;
      dst_address = LINES (dst)[ydst]+xbeg;

      for (xdst=xbeg; xdst<=xend; xdst++) {
	if (*src_address) {
	  if (color_map)
	    *dst_address = color_map->data[*src_address][*dst_address];
	  else
	    *dst_address = (*src_address);
	}

	dst_address++;
	src_address++;
      }
    }
  }
#endif
}

static void alleg_hline(Image *image, int x1, int y, int x2, int color)
{
  hline(image->bmp, x1, y, x2, color);
}

static void alleg_rectfill(Image *image, int x1, int y1, int x2, int y2, int color)
{
  rectfill(image->bmp, x1, y1, x2, y2, color);
}

static void alleg_to_allegro(const Image *image, BITMAP *bmp, int _x, int _y)
{
  blit(image->bmp, bmp, 0, 0, _x, _y, image->w, image->h);
}

static ImageMethods alleg_methods =
{
  alleg_init,
  alleg_getpixel,
  alleg_putpixel,
  alleg_clear,
  alleg_copy,
  alleg_merge,
  alleg_hline,
  alleg_rectfill,
  alleg_to_allegro,
};
