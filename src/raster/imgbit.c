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
 */

#undef BYTES
#undef LINES

#define BYTES(image)   ((ase_uint8 *)image->dat)
#define LINES(image)   ((ase_uint8 **)image->line)

#define BITMAP_HLINE(op)			\
  for (x=x1; x<=x2; x++) {			\
    *address op (1<<d.rem);			\
    _image_bitmap_next_bit(d, address);		\
  }

static int bitmap_regenerate_lines(Image *image)
{
  ase_uint8 *address = BYTES(image);
  int y;

  if (LINES(image))
    jfree(LINES(image));

  image->line = jmalloc(sizeof(ase_uint8 *) * image->h);
  if (!LINES(image))
    return -1;

  for (y=0; y<image->h; y++) {
    LINES(image)[y] = address;
    address += (image->w+7)/8;
  }

  return 0;
}

static int bitmap_init(Image *image)
{
  image->dat = jmalloc(sizeof(ase_uint8) * ((image->w+7)/8) * image->h);
  if (!BYTES(image))
    return -1;

  if (bitmap_regenerate_lines(image) < 0) {
    jfree(BYTES(image));
    return -1;
  }

  return 0;
}

static int bitmap_getpixel(const Image *image, int x, int y)
{
  div_t d = div (x, 8);
  return ((*(LINES (image)[y]+d.quot)) & (1<<d.rem)) ? 1: 0;
}

static void bitmap_putpixel(Image *image, int x, int y, int color)
{
  div_t d = div (x, 8);

  if (color)
    *(LINES (image)[y]+d.quot) |= (1<<d.rem);
  else
    *(LINES (image)[y]+d.quot) &= ~(1<<d.rem);
}

static void bitmap_clear(Image *image, int color)
{
  memset(BYTES(image), color ? 0xff: 0x00, ((image->w+7)/8) * image->h);
}

static void bitmap_copy(Image *dst, const Image *src, int x, int y)
{
  ase_uint8 *src_address;
  ase_uint8 *dst_address;
  int xbeg, xend, xsrc, xdst;
  int ybeg, yend, ysrc, ydst;
  div_t src_d, src_beg_d;
  div_t dst_d, dst_beg_d;

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

  src_beg_d = div (xsrc, 8);
  dst_beg_d = div (xbeg, 8);

  for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
    src_d = src_beg_d;
    dst_d = dst_beg_d;

    src_address = LINES (src)[ysrc]+src_d.quot;
    dst_address = LINES (dst)[ydst]+dst_d.quot;

    for (xdst=xbeg; xdst<=xend; xdst++) {
      if ((*src_address & (1<<src_d.rem)))
        *dst_address |= (1<<dst_d.rem);
      else
        *dst_address &= ~(1<<dst_d.rem);

      _image_bitmap_next_bit (src_d, src_address);
      _image_bitmap_next_bit (dst_d, dst_address);
    }
  }
}

static void bitmap_merge(Image *dst, const Image *src,
			 int x, int y, int opacity, int blend_mode)
{
  ase_uint8 *src_address;
  ase_uint8 *dst_address;
  int xbeg, xend, xsrc, xdst;
  int ybeg, yend, ysrc, ydst;
  div_t src_d, src_beg_d;
  div_t dst_d, dst_beg_d;

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

  src_beg_d = div (xsrc, 8);
  dst_beg_d = div (xbeg, 8);

  for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
    src_d = src_beg_d;
    dst_d = dst_beg_d;

    src_address = LINES (src)[ysrc]+src_d.quot;
    dst_address = LINES (dst)[ydst]+dst_d.quot;

    for (xdst=xbeg; xdst<=xend; xdst++) {
      if ((*src_address & (1<<src_d.rem)))
        *dst_address |= (1<<dst_d.rem);

      _image_bitmap_next_bit (src_d, src_address);
      _image_bitmap_next_bit (dst_d, dst_address);
    }
  }
}

static void bitmap_hline(Image *image, int x1, int y, int x2, int color)
{
  ase_uint8 *address;
  div_t d = div (x1, 8);
  int x;

  address = LINES (image)[y]+d.quot;

  if (color) {
    BITMAP_HLINE( |= );
  }
  else {
    BITMAP_HLINE( &= ~ );
  }
}

static void bitmap_rectfill(Image *image, int x1, int y1, int x2, int y2, int color)
{
  ase_uint8 *address;
  div_t d, beg_d = div (x1, 8);
  int x, y;

  if (color) {
    for (y=y1; y<=y2; y++) {
      d = beg_d;
      address = LINES (image)[y]+d.quot;
      BITMAP_HLINE( |= );
    }
  }
  else {
    for (y=y1; y<=y2; y++) {
      d = beg_d;
      address = LINES (image)[y]+d.quot;
      BITMAP_HLINE( &= ~ );
    }
  }
}

static void bitmap_to_allegro(const Image *image, BITMAP *bmp, int _x, int _y)
{
  ase_uint8 *address;
  unsigned long bmp_address;
  int depth = bitmap_color_depth (bmp);
  div_t d, beg_d = div (0, 8);
  int color[2];
  int x, y;

  bmp_select (bmp);

  switch (depth) {

    case 8:
      color[0] = makecol8 (0, 0, 0);
      color[1] = makecol8 (255, 255, 255);

#if defined GFX_MODEX && !defined ALLEGRO_UNIX
      if (is_planar_bitmap (bmp)) {
        for (y=0; y<image->h; y++) {
	  address = LINES(image)[y];
          bmp_address = (unsigned long)bmp->line[_y];

	  d = beg_d;
          for (x=0; x<image->w; x++) {
            outportw (0x3C4, (0x100<<((_x+x)&3))|2);
            bmp_write8(bmp_address+((_x+x)>>2),
		       color[((*address) & (1<<d.rem))? 1: 0]);
	    _image_bitmap_next_bit(d, address);
          }

          _y++;
        }
      }
      else {
#endif
        for (y=0; y<image->h; y++) {
	  address = LINES (image)[y];
          bmp_address = bmp_write_line(bmp, _y)+_x;

	  d = beg_d;
          for (x=0; x<image->w; x++) {
            bmp_write8 (bmp_address++, color[((*address) & (1<<d.rem))? 1: 0]);
	    _image_bitmap_next_bit(d, address);
          }

          _y++;
        }
#if defined GFX_MODEX && !defined ALLEGRO_UNIX
      }
#endif
      break;

    case 15:
      color[0] = makecol15(0, 0, 0);
      color[1] = makecol15(255, 255, 255);

      _x <<= 1;

      for (y=0; y<image->h; y++) {
	address = LINES (image)[y];
        bmp_address = bmp_write_line(bmp, _y)+_x;

	d = beg_d;
        for (x=0; x<image->w; x++) {
          bmp_write15(bmp_address, color[((*address) & (1<<d.rem))? 1: 0]);
          bmp_address += 2;
	  _image_bitmap_next_bit(d, address);
        }

        _y++;
      }
      break;

    case 16:
      color[0] = makecol16(0, 0, 0);
      color[1] = makecol16(255, 255, 255);

      _x <<= 1;

      for (y=0; y<image->h; y++) {
	address = LINES (image)[y];
        bmp_address = bmp_write_line(bmp, _y)+_x;

	d = beg_d;
        for (x=0; x<image->w; x++) {
          bmp_write16(bmp_address, color[((*address) & (1<<d.rem))? 1: 0]);
          bmp_address += 2;
	  _image_bitmap_next_bit(d, address);
        }

        _y++;
      }
      break;

    case 24:
      color[0] = makecol24 (0, 0, 0);
      color[1] = makecol24 (255, 255, 255);

      _x *= 3;

      for (y=0; y<image->h; y++) {
	address = LINES (image)[y];
        bmp_address = bmp_write_line(bmp, _y)+_x;

	d = beg_d;
        for (x=0; x<image->w; x++) {
          bmp_write24(bmp_address, color[((*address) & (1<<d.rem))? 1: 0]);
          bmp_address += 3;
	  _image_bitmap_next_bit (d, address);
        }

        _y++;
      }
      break;

    case 32:
      color[0] = makeacol32 (0, 0, 0, 255);
      color[1] = makeacol32 (255, 255, 255, 255);

      _x <<= 2;

      for (y=0; y<image->h; y++) {
	address = LINES (image)[y];
        bmp_address = bmp_write_line (bmp, _y)+_x;

	d = beg_d;
        for (x=0; x<image->w; x++) {
          bmp_write32 (bmp_address, color[((*address) & (1<<d.rem))? 1: 0]);
          bmp_address += 4;
	  _image_bitmap_next_bit (d, address);
        }

        _y++;
      }
      break;
  }

  bmp_unwrite_line (bmp);
}

static ImageMethods bitmap_methods =
{
  bitmap_init,
  bitmap_getpixel,
  bitmap_putpixel,
  bitmap_clear,
  bitmap_copy,
  bitmap_merge,
  bitmap_hline,
  bitmap_rectfill,
  bitmap_to_allegro,
};
