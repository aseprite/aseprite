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

#define BYTES(image)   ((unsigned short *)image->dat)
#define LINES(image)   ((unsigned short **)image->line)

static int grayscale_regenerate_lines (Image *image)
{
  unsigned short *address = BYTES (image);
  int y;

  if (LINES (image))
    jfree (LINES (image));

  image->line = jmalloc (sizeof (unsigned short *) * image->h);
  if (!LINES (image))
    return -1;

  for (y=0; y<image->h; y++) {
    LINES (image)[y] = address;
    address += image->w;
  }

  return 0;
}

static int grayscale_init (Image *image)
{
  image->dat = jmalloc (sizeof (unsigned short) * image->w * image->h);
  if (!BYTES (image))
    return -1;

  if (grayscale_regenerate_lines (image) < 0) {
    jfree (BYTES (image));
    return -1;
  }

  return 0;
}

static int grayscale_getpixel (const Image *image, int x, int y)
{
  return *(LINES (image)[y]+x);
}

static void grayscale_putpixel (Image *image, int x, int y, int color)
{
  *(LINES (image)[y]+x) = color;
}

static void grayscale_clear (Image *image, int color)
{
  unsigned short *address = BYTES (image);
  int c, size = image->w * image->h;

  for (c=0; c<size; c++)
    *(address++) = color;
}

static void grayscale_copy (Image *dst, const Image *src, int x, int y)
{
  unsigned short *src_address;
  unsigned short *dst_address;
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

  bytes = (xend - xbeg + 1) << 1;

  for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
    src_address = LINES (src)[ysrc]+xsrc;
    dst_address = LINES (dst)[ydst]+xbeg;

    memcpy (dst_address, src_address, bytes);
  }
}

static void grayscale_merge (Image *dst, const Image *src,
			     int x, int y, int opacity, int blend_mode)
{
  BLEND_COLOR blender = _graya_blenders[blend_mode];
  unsigned short *src_address;
  unsigned short *dst_address;
  int xbeg, xend, xsrc, xdst;
  int ybeg, yend, ysrc, ydst;

  /* nothing to do */
  if (!opacity)
    return;

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

  for (ydst=ybeg; ydst<=yend; ydst++, ysrc++) {
    src_address = LINES (src)[ysrc]+xsrc;
    dst_address = LINES (dst)[ydst]+xbeg;

    for (xdst=xbeg; xdst<=xend; xdst++) {
      *dst_address = (*blender) (*dst_address, *src_address, opacity);

      dst_address++;
      src_address++;
    }
  }
}

static void grayscale_hline (Image *image, int x1, int y, int x2, int color)
{
  unsigned short *address = LINES (image)[y]+x1;
  int x;

  for (x=x1; x<=x2; x++)
    *(address++) = color;
}

static void grayscale_rectfill (Image *image, int x1, int y1, int x2, int y2, int color)
{
  unsigned short *address;
  int x, y;

  for (y=y1; y<=y2; y++) {
    address = LINES (image)[y]+x1;
    for (x=x1; x<=x2; x++)
      *(address++) = color;
  }
}

static void grayscale_to_allegro (const Image *image, BITMAP *bmp, int _x, int _y)
{
  unsigned short *address = BYTES (image);
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
            outportw (0x3C4, (0x100<<((_x+x)&3))|2);
            bmp_write8 (bmp_address+((_x+x)>>2),
			_index_cmap[(*address) & 0xff]);
            address++;
          }

          _y++;
        }
      }
      else {
#endif
        for (y=0; y<image->h; y++) {
          bmp_address = bmp_write_line (bmp, _y)+_x;

          for (x=0; x<image->w; x++) {
            bmp_write8 (bmp_address, _index_cmap[(*address) & 0xff]);
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
        bmp_address = bmp_write_line (bmp, _y)+_x;

        for (x=0; x<image->w; x++) {
          bmp_write15 (bmp_address,
		       makecol15 ((*address) & 0xff,
				  (*address) & 0xff,
				  (*address) & 0xff));
          address++;
          bmp_address += 2;
        }

        _y++;
      }
      break;

    case 16:
      _x <<= 1;

      for (y=0; y<image->h; y++) {
        bmp_address = bmp_write_line (bmp, _y)+_x;

        for (x=0; x<image->w; x++) {
          bmp_write16 (bmp_address,
		       makecol16 ((*address) & 0xff,
				  (*address) & 0xff,
				  (*address) & 0xff));
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
          bmp_write24(bmp_address,
		      makecol24((*address) & 0xff,
				(*address) & 0xff,
				(*address) & 0xff));
          address++;
          bmp_address += 3;
        }

        _y++;
      }
      break;

    case 32:
      _x <<= 2;

      for (y=0; y<image->h; y++) {
        bmp_address = bmp_write_line(bmp, _y)+_x;

        for (x=0; x<image->w; x++) {
          bmp_write32(bmp_address,
		      makeacol32((*address) & 0xff,
				 (*address) & 0xff,
				 (*address) & 0xff, 255));
          address++;
          bmp_address += 4;
        }

        _y++;
      }
      break;
  }

  bmp_unwrite_line (bmp);
}

static ImageMethods grayscale_methods =
{
  grayscale_init,
  grayscale_getpixel,
  grayscale_putpixel,
  grayscale_clear,
  grayscale_copy,
  grayscale_merge,
  grayscale_hline,
  grayscale_rectfill,
  grayscale_to_allegro,
};
