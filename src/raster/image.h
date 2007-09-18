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

#ifndef RASTER_IMAGE_H
#define RASTER_IMAGE_H

#include "raster/gfxobj.h"

#define _rgba_r_shift 0
#define _rgba_g_shift 8
#define _rgba_b_shift 16
#define _rgba_a_shift 24
#define _rgba_getr(c) (((c) >> _rgba_r_shift) & 0xff)
#define _rgba_getg(c) (((c) >> _rgba_g_shift) & 0xff)
#define _rgba_getb(c) (((c) >> _rgba_b_shift) & 0xff)
#define _rgba_geta(c) (((c) >> _rgba_a_shift) & 0xff)
#define _rgba(r,g,b,a)				\
  (((r) << _rgba_r_shift) |			\
   ((g) << _rgba_g_shift) |			\
   ((b) << _rgba_b_shift) |			\
   ((a) << _rgba_a_shift))

#define _graya_k_shift 0
#define _graya_a_shift 8
#define _graya_getk(c) (((c) >> _graya_k_shift) & 0xff)
#define _graya_geta(c) (((c) >> _graya_a_shift) & 0xff)
#define _graya(k,a)				\
  (((k) << _graya_k_shift) |			\
   ((a) << _graya_a_shift))

#define _image_bitmap_next_bit(d, a)		\
  if (d.rem < 7)				\
    d.rem++;					\
  else {					\
    a++;					\
    d.rem = 0;					\
  }

/* Image Types */
enum {
  IMAGE_RGB,
  IMAGE_GRAYSCALE,
  IMAGE_INDEXED,
  IMAGE_BITMAP
};

struct BITMAP;
struct ImageMethods;

typedef struct Image Image;

struct Image
{
  GfxObj gfxobj;
  int imgtype;
  int w, h;
  void *dat;			/* pixmap data */
  void **line;			/* start of each scanline */
  struct ImageMethods *method;
  /* struct BITMAP *bmp; */
};

typedef struct ImageMethods {
  int (*init) (Image *image);
  int (*getpixel) (const Image *image, int x, int y);
  void (*putpixel) (Image *image, int x, int y, int color);
  void (*clear) (Image *image, int color);
  void (*copy) (Image *dst, const Image *src, int x, int y);
  void (*merge) (Image *dst, const Image *src, int x, int y, int opacity,
		 int blend_mode);
  void (*hline) (Image *image, int x1, int y, int x2, int color);
  void (*rectfill) (Image *image, int x1, int y1, int x2, int y2, int color);
  void (*to_allegro) (const Image *image, struct BITMAP *bmp, int x, int y);
} ImageMethods;

Image *image_new(int imgtype, int w, int h);
Image *image_new_copy(const Image *image);
void image_free(Image *image);

int image_depth(Image *image);

int image_getpixel(const Image *image, int x, int y);
void image_putpixel(Image *image, int x, int y, int color);

void image_clear(Image *image, int color);

void image_copy(Image *dst, const Image *src, int x, int y);
void image_merge(Image *dst, const Image *src, int x, int y, int opacity,
		 int blend_mode);

Image *image_crop(const Image *image, int x, int y, int w, int h);

void image_hline(Image *image, int x1, int y, int x2, int color);
void image_vline(Image *image, int x, int y1, int y2, int color);
void image_rect(Image *image, int x1, int y1, int x2, int y2, int color);
void image_rectfill(Image *image, int x1, int y1, int x2, int y2, int color);
void image_line(Image *image, int x1, int y1, int x2, int y2, int color);
void image_ellipse(Image *image, int x1, int y1, int x2, int y2, int color);
void image_ellipsefill(Image *image, int x1, int y1, int x2, int y2,
		       int color);

void image_to_allegro(Image *image, struct BITMAP *bmp, int x, int y);

void image_convert(Image *dst, const Image *src);
int image_count_diff(const Image *i1, const Image *i2);

#endif				/* RASTER_IMAGE_H */
