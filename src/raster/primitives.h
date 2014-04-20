/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef RASTER_PRIMITIVES_H_INCLUDED
#define RASTER_PRIMITIVES_H_INCLUDED
#pragma once

#include "raster/color.h"
#include "raster/image_buffer.h"

namespace raster {
  class Image;
  class Palette;
  class Pen;

  color_t get_pixel(const Image* image, int x, int y);
  void put_pixel(Image* image, int x, int y, color_t c);
  void put_pen(Image* image, Pen* pen, int x, int y, color_t fg, color_t bg);

  void clear_image(Image* image, color_t bg);

  void copy_image(Image* dst, const Image* src, int x, int y);
  void composite_image(Image* dst, const Image* src, int x, int y, int opacity, int blend_mode);

  Image* crop_image(const Image* image, int x, int y, int w, int h, color_t bg, const ImageBufferPtr& buffer = ImageBufferPtr());
  void rotate_image(const Image* src, Image* dst, int angle);

  void draw_hline(Image* image, int x1, int y, int x2, color_t c);
  void draw_vline(Image* image, int x, int y1, int y2, color_t c);
  void draw_rect(Image* image, int x1, int y1, int x2, int y2, color_t c);
  void fill_rect(Image* image, int x1, int y1, int x2, int y2, color_t c);
  void blend_rect(Image* image, int x1, int y1, int x2, int y2, color_t c, int opacity);
  void draw_line(Image* image, int x1, int y1, int x2, int y2, color_t c);
  void draw_ellipse(Image* image, int x1, int y1, int x2, int y2, color_t c);
  void fill_ellipse(Image* image, int x1, int y1, int x2, int y2, color_t c);

  int count_diff_between_images(const Image* i1, const Image* i2);

} // namespace raster

#endif
