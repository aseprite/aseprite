/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

/* #define USE_ALLEGRO_IMAGE */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>
/* #include <allegro/color.h> */
/* #include <allegro/gfx.h> */
#include <string.h>

#include "raster/algo.h"
#include "raster/blend.h"
#include "raster/image.h"

#endif

#ifndef USE_ALLEGRO_IMAGE
#include "imgbit.c"
#include "imggray.c"
#include "imgindex.c"
#include "imgrgb.c"

static ImageMethods *image_methods[] =
{
  &rgb_methods,			/* IMAGE_RGB */
  &grayscale_methods,		/* IMAGE_GRAYSCALE */
  &indexed_methods,		/* IMAGE_INDEXED */
  &bitmap_methods,		/* IMAGE_BITMAP */
};
#else
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

#undef BYTES
#undef LINES

#define BYTES(image)   ((unsigned char *)image->dat)
#define LINES(image)   ((unsigned char **)image->line)

static int alleg_init (Image *image)
{
  static int _image_depth[] = { 32, 16, 8, 8 };
  image->bmp = create_bitmap_ex (_image_depth[image->imgtype],
				 image->w, image->h);
  image->dat = image->bmp->dat;
  image->line = image->bmp->line;
  return 0;
}

static int alleg_getpixel (const Image *image, int x, int y)
{
  return getpixel (image->bmp, x, y);
}

static void alleg_putpixel (Image *image, int x, int y, int color)
{
  putpixel (image->bmp, x, y, color);
}

static void alleg_clear (Image *image, int color)
{
  clear_to_color (image->bmp, color);
}

static void alleg_copy (Image *dst, const Image *src, int x, int y)
{
  blit (src->bmp, dst->bmp, 0, 0, x, y, src->w, src->h);
#if 0
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
#endif
}

/* if "color_map" is not NULL, it's used by the routine to merge the
   source and the destionation pixels */
static void alleg_merge (Image *dst, const Image *src,
			 int x, int y, int opacity, int blend_mode)
{
  masked_blit (src->bmp, dst->bmp, 0, 0, x, y, src->w, src->h);
#if 0
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
#endif
}

static void alleg_hline (Image *image, int x1, int y, int x2, int color)
{
  hline (image->bmp, x1, y, x2, color);
}

static void alleg_rectfill (Image *image, int x1, int y1, int x2, int y2, int color)
{
  rectfill (image->bmp, x1, y1, x2, y2, color);
}

static void alleg_to_allegro (const Image *image, BITMAP *bmp, int _x, int _y)
{
  blit (image->bmp, bmp, 0, 0, _x, _y, image->w, image->h);
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

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
#endif

Image *image_new(int imgtype, int w, int h)
{
  Image *image = (Image *)gfxobj_new(GFXOBJ_IMAGE, sizeof(Image));
  if (!image)
    return NULL;

  image->imgtype = imgtype;
  image->w = w;
  image->h = h;
#ifndef USE_ALLEGRO_IMAGE
  image->method = image_methods[imgtype];
#else
  image->method = &alleg_methods;
#endif
  image->dat = NULL;
  image->line = NULL;
#ifdef USE_ALLEGRO_IMAGE
  image->bmp = NULL;
#endif

  if (image->method)
    if (image->method->init (image) < 0) {
      jfree (image);
      return NULL;
    }

  return image;
}

Image *image_new_copy (const Image *image)
{
  return image_crop (image, 0, 0, image->w, image->h);
}

void image_free (Image *image)
{
#ifndef USE_ALLEGRO_IMAGE
  if (image->dat) jfree(image->dat);
  if (image->line) jfree(image->line);
#else
  destroy_bitmap(image->bmp);
#endif

  gfxobj_free((GfxObj *)image);
}

int image_depth (Image *image)
{
  switch (image->imgtype) {
    case IMAGE_RGB: return 32;
    case IMAGE_GRAYSCALE: return 16;
    case IMAGE_INDEXED: return 8;
    case IMAGE_BITMAP: return 1;
    default: return -1;
  }
}

int image_getpixel (const Image *image, int x, int y)
{
  if ((x >= 0) && (y >= 0) && (x < image->w) && (y < image->h))
    return image->method->getpixel (image, x, y);
  else
    return -1;
}

void image_putpixel (Image *image, int x, int y, int color)
{
  if ((x >= 0) && (y >= 0) && (x < image->w) && (y < image->h))
    image->method->putpixel (image, x, y, color);
}

void image_clear (Image *image, int color)
{
  image->method->clear (image, color);
}

void image_copy (Image *dst, const Image *src, int x, int y)
{
  dst->method->copy (dst, src, x, y);
}

void image_merge (Image *dst, const Image *src, int x, int y, int opacity, int blend_mode)
{
  dst->method->merge (dst, src, x, y, opacity, blend_mode);
}

Image *image_crop (const Image *image, int x, int y, int w, int h)
{
  Image *trim;

  if ((w < 1) || (h < 1))
    return NULL;

  trim = image_new (image->imgtype, w, h);
  if (!trim)
    return NULL;

  image_clear (trim, 0);
  image_copy (trim, image, -x, -y);

  return trim;
}

void image_hline (Image *image, int x1, int y, int x2, int color)
{
  int t;

  if (x1 > x2) {
    t = x1;
    x1 = x2;
    x2 = t;
  }

  if ((x2 < 0) || (x1 >= image->w) || (y < 0) || (y >= image->h))
    return;

  if (x1 < 0) x1 = 0;
  if (x2 >= image->w) x2 = image->w-1;

  image->method->hline (image, x1, y, x2, color);
}

void image_vline (Image *image, int x, int y1, int y2, int color)
{
  int t;

  if (y1 > y2) {
    t = y1;
    y1 = y2;
    y2 = t;
  }

  if ((y2 < 0) || (y1 >= image->h) || (x < 0) || (x >= image->w))
    return;

  if (y1 < 0) y1 = 0;
  if (y2 >= image->h) y2 = image->h-1;

  for (t=y1; t<=y2; t++)
    image->method->putpixel (image, x, t, color);
}

void image_rect (Image *image, int x1, int y1, int x2, int y2, int color)
{
  int t;

  if (x1 > x2) {
    t = x1;
    x1 = x2;
    x2 = t;
  }

  if (y1 > y2) {
    t = y1;
    y1 = y2;
    y2 = t;
  }

  if ((x2 < 0) || (x1 >= image->w) || (y2 < 0) || (y1 >= image->h))
    return;

  image_hline (image, x1, y1, x2, color);
  image_hline (image, x1, y2, x2, color);
  if (y2-y1 > 1) {
    image_vline (image, x1, y1+1, y2-1, color);
    image_vline (image, x2, y1+1, y2-1, color);
  }
}

void image_rectfill (Image *image, int x1, int y1, int x2, int y2, int color)
{
  int t;

  if (x1 > x2) {
    t = x1;
    x1 = x2;
    x2 = t;
  }

  if (y1 > y2) {
    t = y1;
    y1 = y2;
    y2 = t;
  }

  if ((x2 < 0) || (x1 >= image->w) || (y2 < 0) || (y1 >= image->h))
    return;

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= image->w) x2 = image->w-1;
  if (y2 >= image->h) y2 = image->h-1;

  image->method->rectfill (image, x1, y1, x2, y2, color);
}

typedef struct Data
{
  Image *image;
  int color;
} Data;

static void pixel_for_image (int x, int y, Data *data)
{
  image_putpixel (data->image, x, y, data->color);
}

static void hline_for_image (int x1, int y, int x2, Data *data)
{
  image_hline (data->image, x1, y, x2, data->color);
}

void image_line (Image *image, int x1, int y1, int x2, int y2, int color)
{
  Data data = { image, color };
  algo_line (x1, y1, x2, y2, &data, (AlgoPixel)pixel_for_image);
}

void image_ellipse (Image *image, int x1, int y1, int x2, int y2, int color)
{
  Data data = { image, color };
  algo_ellipse (x1, y1, x2, y2, &data, (AlgoPixel)pixel_for_image);
}

void image_ellipsefill (Image *image, int x1, int y1, int x2, int y2, int color)
{
  Data data = { image, color };
  algo_ellipsefill (x1, y1, x2, y2, &data, (AlgoHLine)hline_for_image);
}

void image_to_allegro (Image *image, struct BITMAP *bmp, int x, int y)
{
  image->method->to_allegro (image, bmp, x, y);
}

void image_convert (Image *dst, const Image *src)
{
  int c, x, y, w, h;
  float hue, s, v;

  if ((src->w != dst->w) || (src->h != dst->h))
    return;
  else if (src->imgtype == dst->imgtype) {
    image_copy (dst, src, 0, 0);
    return;
  }

  w = dst->w;
  h = dst->h;

  switch (src->imgtype) {

    case IMAGE_RGB:
      switch (dst->imgtype) {

	/* RGB -> Grayscale */
	case IMAGE_GRAYSCALE:
	  for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	      c = src->method->getpixel (src, x, y);
	      rgb_to_hsv (_rgba_getr (c),
			  _rgba_getg (c),
			  _rgba_getb (c), &hue, &s, &v);
	      v = v * 255.0f;
	      dst->method->putpixel (dst, x, y,
				     _graya((int)MID (0, v, 255),
					    _rgba_geta (c)));
	    }
	  }
	  break;

	/* RGB -> Indexed */
	case IMAGE_INDEXED:
	  for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	      c = src->method->getpixel (src, x, y);
	      if  (!_rgba_geta (c))
		dst->method->putpixel (dst, x, y, 0);
	      else
		dst->method->putpixel (dst, x, y,
				       makecol8 (_rgba_getr (c),
						 _rgba_getg (c),
						 _rgba_getb (c)));
	    }
	  }
	  break;
      }
      break;

    case IMAGE_GRAYSCALE:
      switch (dst->imgtype) {

	/* Grayscale -> RGB */
	case IMAGE_RGB:
	  for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	      c = src->method->getpixel (src, x, y);
	      dst->method->putpixel (dst, x, y,
				     _rgba (_graya_getk (c),
					    _graya_getk (c),
					    _graya_getk (c),
					    _graya_geta (c)));
	    }
	  }
	  break;

	/* Grayscale -> Indexed */
	case IMAGE_INDEXED:
	  for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	      c = src->method->getpixel (src, x, y);
	      if  (!_graya_geta (c))
		dst->method->putpixel (dst, x, y, 0);
	      else
		dst->method->putpixel (dst, x, y,
				       makecol8 (_graya_getk (c),
						 _graya_getk (c),
						 _graya_getk (c)));
	    }
	  }
	  break;
      }
      break;

    case IMAGE_INDEXED:
      switch (dst->imgtype) {

	/* Indexed -> RGB */
	case IMAGE_RGB:
	  for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	      c = src->method->getpixel (src, x, y);
	      if (!c)
		dst->method->putpixel (dst, x, y, 0);
	      else
		dst->method->putpixel (dst, x, y,
				       _rgba (getr8 (c),
					      getg8 (c),
					      getb8 (c), 255));
	    }
	  }
	  break;

	/* Indexed -> Grayscale */
	case IMAGE_GRAYSCALE:
	  for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	      c = src->method->getpixel (src, x, y);
	      if (!c)
		dst->method->putpixel (dst, x, y, 0);
	      else {
		rgb_to_hsv (getr8 (c), getg8 (c), getb8 (c), &hue, &s, &v);
		v = v * 255.0f;
		dst->method->putpixel (dst, x, y,
				       _graya ((int)MID (0, v, 255), 255));
	      }
	    }
	  }
	  break;
      }
      break;
  }
}

int image_count_diff (const Image *i1, const Image *i2)
{
  int c, size, diff = 0;

  if ((i1->imgtype != i2->imgtype) || (i1->w != i2->w) || (i1->h != i2->h))
    return -1;

  size = i1->w * i1->h;

  switch (i1->imgtype) {

    case IMAGE_RGB:
      {
	unsigned long *address1 = (unsigned long *)i1->dat;
	unsigned long *address2 = (unsigned long *)i2->dat;
	for (c=0; c<size; c++)
	  if (*(address1++) != *(address2++))
	    diff++;
      }
      break;

    case IMAGE_GRAYSCALE:
      {
	unsigned short *address1 = (unsigned short *)i1->dat;
	unsigned short *address2 = (unsigned short *)i2->dat;
	for (c=0; c<size; c++)
	  if (*(address1++) != *(address2++))
	    diff++;
      }
      break;

    case IMAGE_INDEXED:
      {
	unsigned char *address1 = (unsigned char *)i1->dat;
	unsigned char *address2 = (unsigned char *)i2->dat;
	for (c=0; c<size; c++)
	  if (*(address1++) != *(address2++))
	    diff++;
      }
      break;

    case IMAGE_BITMAP:
      /* XXX test it */
      {
	unsigned char *address1 = (unsigned char *)i1->dat;
	unsigned char *address2 = (unsigned char *)i2->dat;
	div_t d1 = div (0, 8);
	div_t d2 = div (0, 8);
	for (c=0; c<size; c++) {
	  if (((*address1) & (1<<d1.rem)) !=
	      ((*address2) & (1<<d2.rem)))
	    diff++;
	  _image_bitmap_next_bit (d1, address1);
	  _image_bitmap_next_bit (d2, address2);
	}
      }
      break;
  }

  return diff;
}

/* void image_swap (Image *i1, Image *i2, int x, int y) */
/* { */
/*   int u, v; */

/*   switch (i1->imgtype) { */

/*     case IMAGE_RGB: */
/*       { */
/* 	unsigned long *address1 = (unsigned long *)i1->bytes; */
/* 	unsigned long *address2 = (unsigned long *)i2->bytes; */
/* 	unsigned long aux; */
/* 	for (v=0; v<i2->h; v++) { */
/* 	  address1 = ((unsigned long **)i1->lines)[v+y]+x; */
/* 	  address2 = ((unsigned long **)i2->lines)[v]; */
/* 	  for (u=0; u<i2->w; u++) { */
/* 	    aux = *address1; */
/* 	    *(address1++) = *address2; */
/* 	    *(address2++) = aux; */
/* 	  } */
/* 	} */
/*       } */
/*       break; */

/*     case IMAGE_GRAYSCALE: */
/*       { */
/* 	unsigned short *address1 = (unsigned short *)i1->bytes; */
/* 	unsigned short *address2 = (unsigned short *)i2->bytes; */
/* 	for (c=0; c<size; c++) */
/* 	  if (*(address1++) != *(address2++)) */
/* 	    diff++; */
/*       } */
/*       break; */

/*     case IMAGE_INDEXED: */
/*       { */
/* 	unsigned char *address1 = (unsigned char *)i1->bytes; */
/* 	unsigned char *address2 = (unsigned char *)i2->bytes; */
/* 	for (c=0; c<size; c++) */
/* 	  if (*(address1++) != *(address2++)) */
/* 	    diff++; */
/*       } */
/*       break; */

/*     case IMAGE_BITMAP: */
/*       /\* XXX *\/ */
/*       break; */
/*   } */
/* } */

