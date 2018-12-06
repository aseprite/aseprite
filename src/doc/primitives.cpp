// Aseprite Document Library
// Copyright (c) 2018 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/primitives.h"

#include "doc/algo.h"
#include "doc/brush.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "doc/rgbmap.h"

#include <stdexcept>

namespace doc {

color_t get_pixel(const Image* image, int x, int y)
{
  ASSERT(image);

  if ((x >= 0) && (y >= 0) && (x < image->width()) && (y < image->height()))
    return image->getPixel(x, y);
  else
    return -1;
}

void put_pixel(Image* image, int x, int y, color_t color)
{
  ASSERT(image);

  if ((x >= 0) && (y >= 0) && (x < image->width()) && (y < image->height()))
    image->putPixel(x, y, color);
}

void clear_image(Image* image, color_t color)
{
  ASSERT(image);

  image->clear(color);
}

void copy_image(Image* dst, const Image* src)
{
  ASSERT(dst);
  ASSERT(src);

  dst->copy(src, gfx::Clip(0, 0, 0, 0, src->width(), src->height()));
}

void copy_image(Image* dst, const Image* src, int x, int y)
{
  ASSERT(dst);
  ASSERT(src);

  dst->copy(src, gfx::Clip(x, y, 0, 0, src->width(), src->height()));
}

Image* crop_image(const Image* image, int x, int y, int w, int h, color_t bg, const ImageBufferPtr& buffer)
{
  ASSERT(image);

  if (w < 1) throw std::invalid_argument("crop_image: Width is less than 1");
  if (h < 1) throw std::invalid_argument("crop_image: Height is less than 1");

  Image* trim = Image::create(image->pixelFormat(), w, h, buffer);
  trim->setMaskColor(image->maskColor());

  clear_image(trim, bg);
  trim->copy(image, gfx::Clip(0, 0, x, y, w, h));

  return trim;
}

Image* crop_image(const Image* image, const gfx::Rect& bounds, color_t bg, const ImageBufferPtr& buffer)
{
  return crop_image(image, bounds.x, bounds.y, bounds.w, bounds.h, bg, buffer);
}

void rotate_image(const Image* src, Image* dst, int angle)
{
  ASSERT(src);
  ASSERT(dst);
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
  ASSERT(image);
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
  ASSERT(image);
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
  ASSERT(image);
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
  ASSERT(image);
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

void fill_rect(Image* image, const gfx::Rect& rc, color_t c)
{
  ASSERT(image);

  gfx::Rect clip = rc.createIntersection(image->bounds());
  if (!clip.isEmpty())
    image->fillRect(clip.x, clip.y,
      clip.x+clip.w-1, clip.y+clip.h-1, c);
}

void blend_rect(Image* image, int x1, int y1, int x2, int y2, color_t color, int opacity)
{
  ASSERT(image);
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
  algo_line_continuous(x1, y1, x2, y2, &data, (AlgoPixel)pixel_for_image);
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
bool is_plain_image_templ(const Image* img, const color_t color)
{
  const LockImageBits<ImageTraits> bits(img);
  typename LockImageBits<ImageTraits>::const_iterator it, end;
  for (it=bits.begin(), end=bits.end(); it!=end; ++it) {
    if (*it != color)
      return false;
  }
  ASSERT(it == end);
  return true;
}

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
    if (!ImageTraits::same_color(*it1, *it2))
      diff++;
  }
  ASSERT(it1 == end1);
  ASSERT(it2 == end2);
  return diff;
}

template<typename ImageTraits>
int is_same_image_templ(const Image* i1, const Image* i2)
{
  const LockImageBits<ImageTraits> bits1(i1);
  const LockImageBits<ImageTraits> bits2(i2);
  typename LockImageBits<ImageTraits>::const_iterator it1, it2, end1, end2;
  for (it1 = bits1.begin(), end1 = bits1.end(),
       it2 = bits2.begin(), end2 = bits2.end();
       it1 != end1 && it2 != end2; ++it1, ++it2) {
    if (!ImageTraits::same_color(*it1, *it2))
      return false;
  }
  ASSERT(it1 == end1);
  ASSERT(it2 == end2);
  return true;
}

} // anonymous namespace

bool is_plain_image(const Image* img, color_t c)
{
  switch (img->pixelFormat()) {
    case IMAGE_RGB:       return is_plain_image_templ<RgbTraits>(img, c);
    case IMAGE_GRAYSCALE: return is_plain_image_templ<GrayscaleTraits>(img, c);
    case IMAGE_INDEXED:   return is_plain_image_templ<IndexedTraits>(img, c);
    case IMAGE_BITMAP:    return is_plain_image_templ<BitmapTraits>(img, c);
  }
  return false;
}

bool is_empty_image(const Image* img)
{
  color_t c = 0;                // alpha = 0
  if (img->colorMode() == ColorMode::INDEXED)
    c = img->maskColor();
  return is_plain_image(img, c);
}

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

bool is_same_image(const Image* i1, const Image* i2)
{
  if ((i1->pixelFormat() != i2->pixelFormat()) ||
      (i1->width() != i2->width()) ||
      (i1->height() != i2->height()))
    return false;

  switch (i1->pixelFormat()) {
    case IMAGE_RGB:       return is_same_image_templ<RgbTraits>(i1, i2);
    case IMAGE_GRAYSCALE: return is_same_image_templ<GrayscaleTraits>(i1, i2);
    case IMAGE_INDEXED:   return is_same_image_templ<IndexedTraits>(i1, i2);
    case IMAGE_BITMAP:    return is_same_image_templ<BitmapTraits>(i1, i2);
  }

  ASSERT(false);
  return false;
}

void remap_image(Image* image, const Remap& remap)
{
  ASSERT(image->pixelFormat() == IMAGE_INDEXED);
  if (image->pixelFormat() != IMAGE_INDEXED)
    return;

  LockImageBits<IndexedTraits> bits(image);
  LockImageBits<IndexedTraits>::iterator
    it = bits.begin(),
    end = bits.end();

  for (; it != end; ++it)
    *it = remap[*it];
}

} // namespace doc
