// Aseprite Document Library
// Copyright (c) 2018-2019 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_PRIMITIVES_H_INCLUDED
#define DOC_PRIMITIVES_H_INCLUDED
#pragma once

#include "base/ints.h"
#include "doc/color.h"
#include "doc/image_buffer.h"
#include "gfx/fwd.h"

namespace doc {
  class Brush;
  class Image;
  class Palette;
  class Remap;

  color_t get_pixel(const Image* image, int x, int y);
  void put_pixel(Image* image, int x, int y, color_t c);

  void clear_image(Image* image, color_t bg);

  void copy_image(Image* dst, const Image* src);
  void copy_image(Image* dst, const Image* src, int x, int y);
  Image* crop_image(const Image* image, int x, int y, int w, int h, color_t bg, const ImageBufferPtr& buffer = ImageBufferPtr());
  Image* crop_image(const Image* image, const gfx::Rect& bounds, color_t bg, const ImageBufferPtr& buffer = ImageBufferPtr());
  void rotate_image(const Image* src, Image* dst, int angle);

  void draw_hline(Image* image, int x1, int y, int x2, color_t c);
  void draw_vline(Image* image, int x, int y1, int y2, color_t c);
  void draw_rect(Image* image, int x1, int y1, int x2, int y2, color_t c);
  void fill_rect(Image* image, int x1, int y1, int x2, int y2, color_t c);
  void fill_rect(Image* image, const gfx::Rect& rc, color_t c);
  void blend_rect(Image* image, int x1, int y1, int x2, int y2, color_t c, int opacity);
  void draw_line(Image* image, int x1, int y1, int x2, int y2, color_t c);
  void draw_ellipse(Image* image, int x1, int y1, int x2, int y2, color_t c);
  void fill_ellipse(Image* image, int x1, int y1, int x2, int y2, color_t c);

  bool is_plain_image(const Image* img, color_t c);
  bool is_empty_image(const Image* img);

  int count_diff_between_images(const Image* i1, const Image* i2);
  bool is_same_image(const Image* i1, const Image* i2);

  void remap_image(Image* image, const Remap& remap);

  uint32_t calculate_image_hash(const Image* image,
                                const gfx::Rect& bounds);

} // namespace doc

#endif
