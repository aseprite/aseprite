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

#include "config.h"

#include <assert.h>
#include <allegro.h>
/* #include <allegro/color.h> */
/* #include <allegro/gfx.h> */
#include <string.h>
#include <stdexcept>

#include "raster/algo.h"
#include "raster/blend.h"
#include "raster/brush.h"
#include "raster/image.h"

#ifndef USE_ALLEGRO_IMAGE
#include "imgbit.cpp"
#include "imggray.cpp"
#include "imgindex.cpp"
#include "imgrgb.cpp"

static ImageMethods *image_methods[] =
{
  &rgb_methods,			/* IMAGE_RGB */
  &grayscale_methods,		/* IMAGE_GRAYSCALE */
  &indexed_methods,		/* IMAGE_INDEXED */
  &bitmap_methods,		/* IMAGE_BITMAP */
};
#else
#include "imgalleg.c"
#endif

//////////////////////////////////////////////////////////////////////


Image::Image(int imgtype, int w, int h)
  : GfxObj(GFXOBJ_IMAGE)
{
  this->imgtype = imgtype;
  this->w = w;
  this->h = h;
#ifndef USE_ALLEGRO_IMAGE
  this->method = image_methods[imgtype];
#else
  this->method = &alleg_methods;
#endif
  this->dat = NULL;
  this->line = NULL;
#ifdef USE_ALLEGRO_IMAGE
  this->bmp = NULL;
#endif

  assert(this->method);
  this->method->init(this);
}

Image::~Image()
{
#ifndef USE_ALLEGRO_IMAGE
  if (this->dat) delete this->dat;
  if (this->line) delete this->line;
#else
  destroy_bitmap(this->bmp);
#endif
}

//////////////////////////////////////////////////////////////////////

Image* image_new(int imgtype, int w, int h)
{
  return new Image(imgtype, w, h);
}

Image* image_new_copy(const Image* image)
{
  assert(image);
  return image_crop(image, 0, 0, image->w, image->h, 0);
}

void image_free(Image* image)
{
  assert(image);
  delete image;
}

int image_depth(Image* image)
{
  switch (image->imgtype) {
    case IMAGE_RGB: return 32;
    case IMAGE_GRAYSCALE: return 16;
    case IMAGE_INDEXED: return 8;
    case IMAGE_BITMAP: return 1;
    default: return -1;
  }
}

int image_getpixel(const Image* image, int x, int y)
{
  if ((x >= 0) && (y >= 0) && (x < image->w) && (y < image->h))
    return image->method->getpixel(image, x, y);
  else
    return -1;
}

void image_putpixel(Image* image, int x, int y, int color)
{
  if ((x >= 0) && (y >= 0) && (x < image->w) && (y < image->h))
    image->method->putpixel (image, x, y, color);
}

void image_clear(Image* image, int color)
{
  image->method->clear (image, color);
}

void image_copy(Image* dst, const Image* src, int x, int y)
{
  dst->method->copy (dst, src, x, y);
}

void image_merge(Image* dst, const Image* src, int x, int y, int opacity, int blend_mode)
{
  dst->method->merge(dst, src, x, y, opacity, blend_mode);
}

Image* image_crop(const Image* image, int x, int y, int w, int h, int bgcolor)
{
  if (w < 1) throw std::invalid_argument("image_crop: Width is less than 1");
  if (h < 1) throw std::invalid_argument("image_crop: Height is less than 1");

  Image* trim = image_new(image->imgtype, w, h);

  image_clear(trim, bgcolor);
  image_copy(trim, image, -x, -y);

  return trim;
}

void image_hline(Image* image, int x1, int y, int x2, int color)
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

  image->method->hline(image, x1, y, x2, color);
}

void image_vline(Image* image, int x, int y1, int y2, int color)
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
    image->method->putpixel(image, x, t, color);
}

void image_rect(Image* image, int x1, int y1, int x2, int y2, int color)
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

  image_hline(image, x1, y1, x2, color);
  image_hline(image, x1, y2, x2, color);
  if (y2-y1 > 1) {
    image_vline(image, x1, y1+1, y2-1, color);
    image_vline(image, x2, y1+1, y2-1, color);
  }
}

void image_rectfill(Image* image, int x1, int y1, int x2, int y2, int color)
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

  image->method->rectfill(image, x1, y1, x2, y2, color);
}

typedef struct Data
{
  Image* image;
  int color;
} Data;

static void pixel_for_image(int x, int y, Data *data)
{
  image_putpixel(data->image, x, y, data->color);
}

static void hline_for_image(int x1, int y, int x2, Data *data)
{
  image_hline(data->image, x1, y, x2, data->color);
}

void image_line(Image* image, int x1, int y1, int x2, int y2, int color)
{
  Data data = { image, color };
  algo_line(x1, y1, x2, y2, &data, (AlgoPixel)pixel_for_image);
}

void image_ellipse(Image* image, int x1, int y1, int x2, int y2, int color)
{
  Data data = { image, color };
  algo_ellipse(x1, y1, x2, y2, &data, (AlgoPixel)pixel_for_image);
}

void image_ellipsefill(Image* image, int x1, int y1, int x2, int y2, int color)
{
  Data data = { image, color };
  algo_ellipsefill(x1, y1, x2, y2, &data, (AlgoHLine)hline_for_image);
}

/*********************************************************************
 Brushes
 *********************************************************************/

/* typedef struct AlgoData */
/* { */
/*   Image* image; */
/*   Brush *brush; */
/*   int color; */
/* } AlgoData; */

/* static void algo_putpixel(int x, int y, AlgoData *data); */
/* static void algo_putbrush(int x, int y, AlgoData *data); */

/* void image_putpixel_brush(Image* image, Brush *brush, int x, int y, int color) */
/* { */
/*   AlgoData data = { image, brush, color }; */
/*   if (brush->size == 1) */
/*     algo_putpixel(x, y, &data); */
/*   else */
/*     algo_putbrush(x, y, &data); */
/* } */

/* void image_hline_brush(Image* image, Brush *brush, int x1, int y, int x2, int color) */
/* { */
/*   AlgoData data = { image, brush, color }; */
/*   int x; */

/*   if (brush->size == 1) */
/*     for (x=x1; x<=x2; ++x) */
/*       algo_putpixel(x, y, &data); */
/*   else */
/*     for (x=x1; x<=x2; ++x) */
/*       algo_putbrush(x, y, &data); */
/* } */

/* void image_line_brush(Image* image, Brush *brush, int x1, int y1, int x2, int y2, int color) */
/* { */
/*   AlgoData data = { image, brush, color }; */
/*   algo_line(x1, y1, x2, y2, &data, */
/* 	    (brush->size == 1)? */
/* 	    (AlgoPixel)algo_putpixel: */
/* 	    (AlgoPixel)algo_putbrush); */
/* } */

/* static void algo_putpixel(int x, int y, AlgoData *data) */
/* { */
/*   image_putpixel(data->image, x, y, data->color); */
/* } */

/* static void algo_putbrush(int x, int y, AlgoData *data) */
/* { */
/*   register struct BrushScanline *scanline = data->brush->scanline; */
/*   register int c = data->brush->size/2; */

/*   x -= c; */
/*   y -= c; */

/*   for (c=0; c<data->brush->size; ++c) { */
/*     if (scanline->state) */
/*       image_hline(data->image, x+scanline->x1, y+c, x+scanline->x2, data->color); */
/*     ++scanline; */
/*   } */
/* } */

/*********************************************************************
 Allegro <-> Image
 *********************************************************************/

void image_to_allegro(Image* image, BITMAP *bmp, int x, int y)
{
  image->method->to_allegro(image, bmp, x, y);
}

void image_convert(Image* dst, const Image* src)
{
  int c, x, y, w, h;
  float hue, s, v;

  if ((src->w != dst->w) || (src->h != dst->h))
    return;
  else if (src->imgtype == dst->imgtype) {
    image_copy(dst, src, 0, 0);
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
	      c = src->method->getpixel(src, x, y);
	      rgb_to_hsv(_rgba_getr(c),
			 _rgba_getg(c),
			 _rgba_getb(c), &hue, &s, &v);
	      v = v * 255.0f;
	      dst->method->putpixel(dst, x, y,
				    _graya((int)MID(0, v, 255),
					   _rgba_geta(c)));
	    }
	  }
	  break;

	/* RGB -> Indexed */
	case IMAGE_INDEXED:
	  for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	      c = src->method->getpixel(src, x, y);
	      if  (!_rgba_geta (c))
		dst->method->putpixel(dst, x, y, 0);
	      else
		dst->method->putpixel(dst, x, y,
				      makecol8(_rgba_getr(c),
					       _rgba_getg(c),
					       _rgba_getb(c)));
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
	      c = src->method->getpixel(src, x, y);
	      dst->method->putpixel(dst, x, y,
				    _rgba(_graya_getv(c),
					  _graya_getv(c),
					  _graya_getv(c),
					  _graya_geta(c)));
	    }
	  }
	  break;

	/* Grayscale -> Indexed */
	case IMAGE_INDEXED:
	  for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	      c = src->method->getpixel(src, x, y);
	      if  (!_graya_geta(c))
		dst->method->putpixel(dst, x, y, 0);
	      else
		dst->method->putpixel(dst, x, y,
				      makecol8(_graya_getv(c),
					       _graya_getv(c),
					       _graya_getv(c)));
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
	      c = src->method->getpixel(src, x, y);
	      if (!c)
		dst->method->putpixel(dst, x, y, 0);
	      else
		dst->method->putpixel(dst, x, y,
				      _rgba(getr8(c),
					    getg8(c),
					    getb8(c), 255));
	    }
	  }
	  break;

	/* Indexed -> Grayscale */
	case IMAGE_GRAYSCALE:
	  for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	      c = src->method->getpixel(src, x, y);
	      if (!c)
		dst->method->putpixel(dst, x, y, 0);
	      else {
		rgb_to_hsv(getr8(c), getg8(c), getb8(c), &hue, &s, &v);
		v = v * 255.0f;
		dst->method->putpixel(dst, x, y,
				      _graya((int)MID(0, v, 255), 255));
	      }
	    }
	  }
	  break;
      }
      break;
  }
}

int image_count_diff(const Image* i1, const Image* i2)
{
  int c, size, diff = 0;

  if ((i1->imgtype != i2->imgtype) || (i1->w != i2->w) || (i1->h != i2->h))
    return -1;

  size = i1->w * i1->h;

  switch (i1->imgtype) {

    case IMAGE_RGB:
      {
	ase_uint32 *address1 = (ase_uint32 *)i1->dat;
	ase_uint32 *address2 = (ase_uint32 *)i2->dat;
	for (c=0; c<size; c++)
	  if (*(address1++) != *(address2++))
	    diff++;
      }
      break;

    case IMAGE_GRAYSCALE:
      {
	ase_uint16 *address1 = (ase_uint16 *)i1->dat;
	ase_uint16 *address2 = (ase_uint16 *)i2->dat;
	for (c=0; c<size; c++)
	  if (*(address1++) != *(address2++))
	    diff++;
      }
      break;

    case IMAGE_INDEXED:
      {
	ase_uint8 *address1 = (ase_uint8 *)i1->dat;
	ase_uint8 *address2 = (ase_uint8 *)i2->dat;
	for (c=0; c<size; c++)
	  if (*(address1++) != *(address2++))
	    diff++;
      }
      break;

    case IMAGE_BITMAP:
      /* TODO test it */
      {
	ase_uint8 *address1 = (ase_uint8 *)i1->dat;
	ase_uint8 *address2 = (ase_uint8 *)i2->dat;
	div_t d1 = div (0, 8);
	div_t d2 = div (0, 8);
	for (c=0; c<size; c++) {
	  if (((*address1) & (1<<d1.rem)) !=
	      ((*address2) & (1<<d2.rem)))
	    diff++;
	  _image_bitmap_next_bit(d1, address1);
	  _image_bitmap_next_bit(d2, address2);
	}
      }
      break;
  }

  return diff;
}

bool image_shrink_rect(Image *image, int *x1, int *y1, int *x2, int *y2, int refpixel)
{
#define SHRINK_SIDE(u_begin, u_op, u_final, u_add,		\
		    v_begin, v_op, v_final, v_add, U, V, var)	\
  do {								\
    for (u = u_begin; u u_op u_final; u u_add) {		\
      for (v = v_begin; v v_op v_final; v v_add) {		\
	if (image->method->getpixel (image, U, V) != refpixel)	\
	  break;						\
      }								\
      if (v == v_final)						\
	var;							\
      else							\
	break;							\
    }								\
  } while (0)

  int u, v;

  *x1 = 0;
  *y1 = 0;
  *x2 = image->w-1;
  *y2 = image->h-1;

  SHRINK_SIDE(0, <, image->w, ++,
	      0, <, image->h, ++, u, v, (*x1)++);

  SHRINK_SIDE(0, <, image->h, ++,
	      0, <, image->w, ++, v, u, (*y1)++);

  SHRINK_SIDE(image->w-1, >, 0, --,
	      0, <, image->h, ++, u, v, (*x2)--);

  SHRINK_SIDE(image->h-1, >, 0, --,
	      0, <, image->w, ++, v, u, (*y2)--);

  if ((*x1 > *x2) || (*y1 > *y2))
    return false;
  else
    return true;

#undef SHRINK_SIDE
}

