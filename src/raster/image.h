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
 */

#ifndef RASTER_IMAGE_H_INCLUDED
#define RASTER_IMAGE_H_INCLUDED

#include <allegro/color.h>
#include "raster/gfxobj.h"
#include "raster/blend.h"

class Palette;
class Pen;
class RgbMap;

// Image Types
enum {
  IMAGE_RGB,
  IMAGE_GRAYSCALE,
  IMAGE_INDEXED,
  IMAGE_BITMAP
};

enum ResizeMethod {
  RESIZE_METHOD_NEAREST_NEIGHBOR,
  RESIZE_METHOD_BILINEAR,
};

struct BITMAP;

class Image : public GfxObj
{
public:
  int imgtype;
  int w, h;
  ase_uint8* dat;		// pixmap data
  ase_uint8** line;		// start of each scanline
  ase_uint32 mask_color;	// skipped color in merge process

  Image(int imgtype, int w, int h);
  virtual ~Image();

  virtual int getpixel(int x, int y) const = 0;
  virtual void putpixel(int x, int y, int color) = 0;
  virtual void clear(int color) = 0;
  virtual void copy(const Image* src, int x, int y) = 0;
  virtual void merge(const Image* src, int x, int y, int opacity, int blend_mode) = 0;
  virtual void hline(int x1, int y, int x2, int color) = 0;
  virtual void rectfill(int x1, int y1, int x2, int y2, int color) = 0;
  virtual void to_allegro(BITMAP* bmp, int x, int y, const Palette* palette) const = 0;
};

Image* image_new(int imgtype, int w, int h);
Image* image_new_copy(const Image* image);
void image_free(Image* image);

int image_depth(Image* image);

int image_getpixel(const Image* image, int x, int y);
void image_putpixel(Image* image, int x, int y, int color);
void image_putpen(Image* image, Pen* pen, int x, int y, int fg_color, int bg_color);

void image_clear(Image* image, int color);

void image_copy(Image* dst, const Image* src, int x, int y);
void image_merge(Image* dst, const Image* src, int x, int y, int opacity,
		 int blend_mode);

Image* image_crop(const Image* image, int x, int y, int w, int h, int bgcolor);
void image_rotate(const Image* src, Image* dst, int angle);

void image_hline(Image* image, int x1, int y, int x2, int color);
void image_vline(Image* image, int x, int y1, int y2, int color);
void image_rect(Image* image, int x1, int y1, int x2, int y2, int color);
void image_rectfill(Image* image, int x1, int y1, int x2, int y2, int color);
void image_line(Image* image, int x1, int y1, int x2, int y2, int color);
void image_ellipse(Image* image, int x1, int y1, int x2, int y2, int color);
void image_ellipsefill(Image* image, int x1, int y1, int x2, int y2, int color);

void image_to_allegro(const Image* image, BITMAP* bmp, int x, int y, const Palette* palette);

void image_fixup_transparent_colors(Image* image);
void image_resize(const Image* src, Image* dst, ResizeMethod method, const Palette* palette, const RgbMap* rgbmap);
int image_count_diff(const Image* i1, const Image* i2);
bool image_shrink_rect(Image *image, int *x1, int *y1, int *x2, int *y2, int refpixel);

inline int image_shift(Image* image)
{
  return ((image->imgtype == IMAGE_RGB)?       2:
	  (image->imgtype == IMAGE_GRAYSCALE)? 1: 0);
}

inline int image_line_size(Image* image, int width)
{
  return (width << image_shift(image));
}

inline void* image_address(Image* image, int x, int y)
{
  return ((void *)(image->line[y] + image_line_size(image, x)));
}

#include "image_traits.h"

#endif
