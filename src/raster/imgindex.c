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
 */

#undef BYTES
#undef LINES

#define BYTES(image)   ((unsigned char *)image->dat)
#define LINES(image)   ((unsigned char **)image->line)

static int indexed_regenerate_lines(Image *image)
{
  unsigned char *address = BYTES(image);
  int y;

  if (LINES(image))
    jfree(LINES(image));

  image->line = jmalloc(sizeof(unsigned char *) * image->h);
  if (!LINES(image))
    return -1;

  for (y=0; y<image->h; y++) {
    LINES(image)[y] = address;
    address += image->w;
  }

  return 0;
}

static int indexed_init(Image *image)
{
  image->dat = jmalloc(sizeof(unsigned char) * image->w * image->h);
  if (!BYTES(image))
    return -1;

  if (indexed_regenerate_lines(image) < 0) {
    jfree(BYTES(image));
    return -1;
  }

  return 0;
}

static int indexed_getpixel(const Image *image, int x, int y)
{
  return *(LINES(image)[y]+x);
}

static void indexed_putpixel(Image *image, int x, int y, int color)
{
  *(LINES(image)[y]+x) = color;
}

static void indexed_clear(Image *image, int color)
{
  memset(BYTES(image), color, image->w*image->h);
}

static void indexed_copy(Image *dst, const Image *src, int x, int y)
{
  unsigned char *src_address;
  unsigned char *dst_address;
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
}

/* if "color_map" is not NULL, it's used by the routine to merge the
   source and the destionation pixels */
static void indexed_merge(Image *dst, const Image *src,
			  int x, int y, int opacity, int blend_mode)
{
  unsigned char *src_address;
  unsigned char *dst_address;
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
}

static void indexed_hline(Image *image, int x1, int y, int x2, int color)
{
  memset (LINES (image)[y]+x1, color, x2-x1+1);
}

static void indexed_rectfill(Image *image, int x1, int y1, int x2, int y2, int color)
{
  int y, bytes = x2-x1+1;

  for (y=y1; y<=y2; y++)
    memset (LINES (image)[y]+x1, color, bytes);
}

static void indexed_to_allegro(const Image *image, BITMAP *bmp, int _x, int _y)
{
#define RGB_TRIPLET						\
  _rgb_scale_6[_current_palette[_index_cmap[(*address)]].r],	\
  _rgb_scale_6[_current_palette[_index_cmap[(*address)]].g],	\
  _rgb_scale_6[_current_palette[_index_cmap[(*address)]].b]

  unsigned char *address = BYTES (image);
  unsigned long bmp_address;
  int depth = bitmap_color_depth (bmp);
  int x, y;

  bmp_select (bmp);

  switch (depth) {

    case 8:
#ifdef GFX_MODEX
      if (is_planar_bitmap (bmp)) {
        for (y=0; y<image->h; y++) {
          bmp_address = (unsigned long)bmp->line[_y];

          for (x=0; x<image->w; x++) {
            outportw(0x3C4, (0x100<<((_x+x)&3))|2);
            bmp_write8(bmp_address+((_x+x)>>2), _index_cmap[(*address)]);
            address++;
          }

          _y++;
        }
      }
      else {
#endif
        for (y=0; y<image->h; y++) {
          bmp_address = bmp_write_line(bmp, _y)+_x;

          for (x=0; x<image->w; x++) {
            bmp_write8(bmp_address, _index_cmap[(*address)]);
            address++;
            bmp_address++;
          }

          _y++;
        }
#ifdef GFX_MODEX
      }
#endif
      break;

    case 15:
      _x <<= 1;

      for (y=0; y<image->h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<image->w; x++) {
          bmp_write15(bmp_address, makecol15 (RGB_TRIPLET));
          address++;
          bmp_address += 2;
        }

        _y++;
      }
      break;

    case 16:
      _x <<= 1;

      for (y=0; y<image->h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<image->w; x++) {
          bmp_write16(bmp_address, makecol16 (RGB_TRIPLET));
          address++;
          bmp_address += 2;
        }

        _y++;
      }
      break;

    case 24:
      _x *= 3;

      for (y=0; y<image->h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<image->w; x++) {
          bmp_write24(bmp_address, makecol24 (RGB_TRIPLET));
          address++;
          bmp_address += 3;
        }

        _y++;
      }
      break;

    case 32:
      _x <<= 2;

      for (y=0; y<image->h; y++) {
        bmp_address = bmp_write_line (bmp, _y)+_x;

        for (x=0; x<image->w; x++) {
          bmp_write32(bmp_address, makeacol32 (RGB_TRIPLET, 255));
          address++;
          bmp_address += 4;
        }

        _y++;
      }
      break;
  }

  bmp_unwrite_line(bmp);
}

static ImageMethods indexed_methods =
{
  indexed_init,
  indexed_getpixel,
  indexed_putpixel,
  indexed_clear,
  indexed_copy,
  indexed_merge,
  indexed_hline,
  indexed_rectfill,
  indexed_to_allegro,
};
