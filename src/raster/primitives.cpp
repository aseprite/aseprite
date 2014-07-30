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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/primitives.h"

#include "raster/algo.h"
#include "raster/blend.h"
#include "raster/brush.h"
#include "raster/image.h"
#include "raster/image_impl.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"

#include <stdexcept>

namespace raster {

color_t get_pixel(const Image* image, int x, int y)
{
  if ((x >= 0) && (y >= 0) && (x < image->width()) && (y < image->height()))
    return image->getPixel(x, y);
  else
    return -1;
}

void put_pixel(Image* image, int x, int y, color_t color)
{
  if ((x >= 0) && (y >= 0) && (x < image->width()) && (y < image->height()))
    image->putPixel(x, y, color);
}

void draw_brush(Image* image, Brush* brush, int x, int y, color_t fg, color_t bg)
{
  Image* brush_image = brush->image();
  const gfx::Rect& brushBounds = brush->bounds();

  x += brushBounds.x;
  y += brushBounds.y;

  if (fg == bg) {
    fill_rect(image, x, y, x+brushBounds.w-1, y+brushBounds.h-1, bg);
  }
  else {
    int u, v;
    for (v=0; v<brushBounds.h; v++) {
      for (u=0; u<brushBounds.w; u++) {
        if (get_pixel(brush_image, u, v))
          put_pixel(image, x+u, y+v, fg);
        else
          put_pixel(image, x+u, y+v, bg);
      }
    }
  }
}

void clear_image(Image* image, color_t color)
{
  image->clear(color);
}

void copy_image(Image* dst, const Image* src, int x, int y)
{
  dst->copy(src, x, y);
}

void composite_image(Image* dst, const Image* src, int x, int y, int opacity, int blend_mode)
{
  dst->merge(src, x, y, opacity, blend_mode);
}

Image* crop_image(const Image* image, int x, int y, int w, int h, color_t bg, const ImageBufferPtr& buffer)
{
  if (w < 1) throw std::invalid_argument("image_crop: Width is less than 1");
  if (h < 1) throw std::invalid_argument("image_crop: Height is less than 1");

  Image* trim = Image::create(image->pixelFormat(), w, h, buffer);
  trim->setMaskColor(image->maskColor());

  clear_image(trim, bg);
  copy_image(trim, image, -x, -y);

  return trim;
}

void rotate_image(const Image* src, Image* dst, int angle)
{
  int x, y;

  switch (angle) {

    case 180:
      ASSERT(dst->width() == src->width());
      ASSERT(dst->height() == src->height());

      for (y=0; y<src->height(); ++y)
        for (x=0; x<src->width(); ++x)
          dst->putPixel(src->width() - x - 1,
                        src->height() - y - 1, src->getPixel(x, y));
      break;

    case 90:
      ASSERT(dst->width() == src->height());
      ASSERT(dst->height() == src->width());

      for (y=0; y<src->height(); ++y)
        for (x=0; x<src->width(); ++x)
          dst->putPixel(src->height() - y - 1, x, src->getPixel(x, y));
      break;

    case -90:
      ASSERT(dst->width() == src->height());
      ASSERT(dst->height() == src->width());

      for (y=0; y<src->height(); ++y)
        for (x=0; x<src->width(); ++x)
          dst->putPixel(y, src->width() - x - 1, src->getPixel(x, y));
      break;

    // bad angle
    default:
      throw std::invalid_argument("Invalid angle specified to rotate the image");
  }
}

void draw_hline(Image* image, int x1, int y, int x2, color_t color)
{
  int t;

  if (x1 > x2) {
    t = x1;
    x1 = x2;
    x2 = t;
  }

  if ((x2 < 0) || (x1 >= image->width()) || (y < 0) || (y >= image->height()))
    return;

  if (x1 < 0) x1 = 0;
  if (x2 >= image->width()) x2 = image->width()-1;

  image->drawHLine(x1, y, x2, color);
}

void draw_vline(Image* image, int x, int y1, int y2, color_t color)
{
  int t;

  if (y1 > y2) {
    t = y1;
    y1 = y2;
    y2 = t;
  }

  if ((y2 < 0) || (y1 >= image->height()) || (x < 0) || (x >= image->width()))
    return;

  if (y1 < 0) y1 = 0;
  if (y2 >= image->height()) y2 = image->height()-1;

  for (t=y1; t<=y2; t++)
    image->putPixel(x, t, color);
}

void draw_rect(Image* image, int x1, int y1, int x2, int y2, color_t color)
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

  if ((x2 < 0) || (x1 >= image->width()) || (y2 < 0) || (y1 >= image->height()))
    return;

  draw_hline(image, x1, y1, x2, color);
  draw_hline(image, x1, y2, x2, color);
  if (y2-y1 > 1) {
    draw_vline(image, x1, y1+1, y2-1, color);
    draw_vline(image, x2, y1+1, y2-1, color);
  }
}

void fill_rect(Image* image, int x1, int y1, int x2, int y2, color_t color)
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

  if ((x2 < 0) || (x1 >= image->width()) || (y2 < 0) || (y1 >= image->height()))
    return;

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= image->width()) x2 = image->width()-1;
  if (y2 >= image->height()) y2 = image->height()-1;

  image->fillRect(x1, y1, x2, y2, color);
}

void blend_rect(Image* image, int x1, int y1, int x2, int y2, color_t color, int opacity)
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

  if ((x2 < 0) || (x1 >= image->width()) || (y2 < 0) || (y1 >= image->height()))
    return;

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= image->width()) x2 = image->width()-1;
  if (y2 >= image->height()) y2 = image->height()-1;

  image->blendRect(x1, y1, x2, y2, color, opacity);
}

struct Data {
  Image* image;
  color_t color;
};

static void pixel_for_image(int x, int y, Data* data)
{
  put_pixel(data->image, x, y, data->color);
}

static void hline_for_image(int x1, int y, int x2, Data* data)
{
  draw_hline(data->image, x1, y, x2, data->color);
}

void draw_line(Image* image, int x1, int y1, int x2, int y2, color_t color)
{
  Data data = { image, color };
  algo_line(x1, y1, x2, y2, &data, (AlgoPixel)pixel_for_image);
}

void draw_ellipse(Image* image, int x1, int y1, int x2, int y2, color_t color)
{
  Data data = { image, color };
  algo_ellipse(x1, y1, x2, y2, &data, (AlgoPixel)pixel_for_image);
}

void fill_ellipse(Image* image, int x1, int y1, int x2, int y2, color_t color)
{
  Data data = { image, color };
  algo_ellipsefill(x1, y1, x2, y2, &data, (AlgoHLine)hline_for_image);
}

namespace {

template<typename ImageTraits>
int count_diff_between_images_templ(const Image* i1, const Image* i2)
{
  int diff = 0;
  const LockImageBits<ImageTraits> bits1(i1);
  const LockImageBits<ImageTraits> bits2(i2);
  typename LockImageBits<ImageTraits>::const_iterator it1, it2, end1, end2;
  for (it1 = bits1.begin(), end1 = bits1.end(),
       it2 = bits2.begin(), end2 = bits2.end();
       it1 != end1 && it2 != end2; ++it1, ++it2) {
    if (*it1 != *it2)
      diff++;
  }

  ASSERT(it1 == end1);
  ASSERT(it2 == end2);
  return diff;
}

} // anonymous namespace

int count_diff_between_images(const Image* i1, const Image* i2)
{
  if ((i1->pixelFormat() != i2->pixelFormat()) ||
      (i1->width() != i2->width()) ||
      (i1->height() != i2->height()))
    return -1;

  switch (i1->pixelFormat()) {
    case IMAGE_RGB:       return count_diff_between_images_templ<RgbTraits>(i1, i2);
    case IMAGE_GRAYSCALE: return count_diff_between_images_templ<GrayscaleTraits>(i1, i2);
    case IMAGE_INDEXED:   return count_diff_between_images_templ<IndexedTraits>(i1, i2);
    case IMAGE_BITMAP:    return count_diff_between_images_templ<BitmapTraits>(i1, i2);
  }

  ASSERT(false);
  return -1;
}

} // namespace raster
