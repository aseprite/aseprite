// Aseprite Document Library
// Copyright (c) 2018-2023 Igara Studio S.A.
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
#include "doc/dispatch.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "doc/rgbmap.h"
#include "doc/tile.h"
#include "gfx/region.h"

#include <city.h>

#include <stdexcept>

#if defined(__x86_64__) || defined(_WIN64)
  #include <emmintrin.h>
#endif

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

void copy_image(Image* dst, const Image* src, const gfx::Region& rgn)
{
  for (const gfx::Rect& rc : rgn)
    dst->copy(src, gfx::Clip(rc));
}

Image*
crop_image(const Image* image, int x, int y, int w, int h, color_t bg, const ImageBufferPtr& buffer)
{
  ASSERT(image);

  if (w < 1)
    throw std::invalid_argument("crop_image: Width is less than 1");
  if (h < 1)
    throw std::invalid_argument("crop_image: Height is less than 1");

  Image* trim = Image::create(image->pixelFormat(), w, h, buffer);
  trim->setMaskColor(image->maskColor());

  clear_image(trim, bg);
  trim->copy(image, gfx::Clip(0, 0, x, y, w, h));

  return trim;
}

Image* crop_image(const Image* image,
                  const gfx::Rect& bounds,
                  color_t bg,
                  const ImageBufferPtr& buffer)
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

      for (y = 0; y < src->height(); ++y)
        for (x = 0; x < src->width(); ++x)
          dst->putPixel(src->width() - x - 1, src->height() - y - 1, src->getPixel(x, y));
      break;

    case 90:
      ASSERT(dst->width() == src->height());
      ASSERT(dst->height() == src->width());

      for (y = 0; y < src->height(); ++y)
        for (x = 0; x < src->width(); ++x)
          dst->putPixel(src->height() - y - 1, x, src->getPixel(x, y));
      break;

    case -90:
      ASSERT(dst->width() == src->height());
      ASSERT(dst->height() == src->width());

      for (y = 0; y < src->height(); ++y)
        for (x = 0; x < src->width(); ++x)
          dst->putPixel(y, src->width() - x - 1, src->getPixel(x, y));
      break;

    // bad angle
    default: throw std::invalid_argument("Invalid angle specified to rotate the image");
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

  if (x1 < 0)
    x1 = 0;
  if (x2 >= image->width())
    x2 = image->width() - 1;

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

  if (y1 < 0)
    y1 = 0;
  if (y2 >= image->height())
    y2 = image->height() - 1;

  for (t = y1; t <= y2; t++)
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
  if (y2 - y1 > 1) {
    draw_vline(image, x1, y1 + 1, y2 - 1, color);
    draw_vline(image, x2, y1 + 1, y2 - 1, color);
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

  if (x1 < 0)
    x1 = 0;
  if (y1 < 0)
    y1 = 0;
  if (x2 >= image->width())
    x2 = image->width() - 1;
  if (y2 >= image->height())
    y2 = image->height() - 1;

  image->fillRect(x1, y1, x2, y2, color);
}

void fill_rect(Image* image, const gfx::Rect& rc, color_t c)
{
  ASSERT(image);

  gfx::Rect clip = rc.createIntersection(image->bounds());
  if (!clip.isEmpty())
    image->fillRect(clip.x, clip.y, clip.x + clip.w - 1, clip.y + clip.h - 1, c);
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

  if (x1 < 0)
    x1 = 0;
  if (y1 < 0)
    y1 = 0;
  if (x2 >= image->width())
    x2 = image->width() - 1;
  if (y2 >= image->height())
    y2 = image->height() - 1;

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

void draw_ellipse(Image* image,
                  int x1,
                  int y1,
                  int x2,
                  int y2,
                  int extraXPxs,
                  int extraYPxs,
                  color_t color)
{
  Data data = { image, color };
  algo_ellipse(x1, y1, x2, y2, extraXPxs, extraYPxs, &data, (AlgoPixel)pixel_for_image);
}

void fill_ellipse(Image* image,
                  int x1,
                  int y1,
                  int x2,
                  int y2,
                  int extraXPxs,
                  int extraYPxs,
                  color_t color)
{
  Data data = { image, color };
  algo_ellipsefill(x1, y1, x2, y2, extraXPxs, extraYPxs, &data, (AlgoHLine)hline_for_image);
}

namespace {

template<typename ImageTraits>
bool is_plain_image_templ(const Image* img, const color_t color)
{
  const LockImageBits<ImageTraits> bits(img);
  typename LockImageBits<ImageTraits>::const_iterator it, end;
  for (it = bits.begin(), end = bits.end(); it != end; ++it) {
    if (!ImageTraits::same_color(*it, color))
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
  for (it1 = bits1.begin(), end1 = bits1.end(), it2 = bits2.begin(), end2 = bits2.end();
       it1 != end1 && it2 != end2;
       ++it1, ++it2) {
    if (!ImageTraits::same_color(*it1, *it2))
      diff++;
  }
  ASSERT(it1 == end1);
  ASSERT(it2 == end2);
  return diff;
}

template<typename ImageTraits>
bool is_same_image_templ(const Image* i1, const Image* i2)
{
  const LockImageBits<ImageTraits> bits1(i1);
  const LockImageBits<ImageTraits> bits2(i2);
  typename LockImageBits<ImageTraits>::const_iterator it1, it2, end1, end2;
  for (it1 = bits1.begin(), end1 = bits1.end(), it2 = bits2.begin(), end2 = bits2.end();
       it1 != end1 && it2 != end2;
       ++it1, ++it2) {
    if (!ImageTraits::same_color(*it1, *it2))
      return false;
  }
  ASSERT(it1 == end1);
  ASSERT(it2 == end2);
  return true;
}

template<typename ImageTraits>
bool is_same_image_simd_templ(const Image* i1, const Image* i2)
{
  using address_t = typename ImageTraits::address_t;
  const int w = i1->width();
  const int h = i1->height();
  for (int y = 0; y < h; ++y) {
    auto p = (const address_t)i1->getPixelAddress(0, y);
    auto q = (const address_t)i2->getPixelAddress(0, y);
    int x = 0;

#if DOC_USE_ALIGNED_PIXELS
  #if defined(__x86_64__) || defined(_WIN64)
    // Use SSE2

    if constexpr (ImageTraits::bytes_per_pixel == 4) {
      for (; x + 4 <= w; x += 4, p += 4, q += 4) {
        __m128i r = _mm_cmpeq_epi32(*(const __m128i*)p, *(const __m128i*)q);
        if (_mm_movemask_epi8(r) != 0xffff) { // !_mm_test_all_ones(r)
          if (!ImageTraits::same_color(p[0], q[0]) || !ImageTraits::same_color(p[1], q[1]) ||
              !ImageTraits::same_color(p[2], q[2]) || !ImageTraits::same_color(p[3], q[3]))
            return false;
        }
      }
    }
    else if constexpr (ImageTraits::bytes_per_pixel == 2) {
      for (; x + 8 <= w; x += 8, p += 8, q += 8) {
        __m128i r = _mm_cmpeq_epi16(*(const __m128i*)p, *(const __m128i*)q);
        if (_mm_movemask_epi8(r) != 0xffff) { // !_mm_test_all_ones(r)
          if (!ImageTraits::same_color(p[0], q[0]) || !ImageTraits::same_color(p[1], q[1]) ||
              !ImageTraits::same_color(p[2], q[2]) || !ImageTraits::same_color(p[3], q[3]) ||
              !ImageTraits::same_color(p[4], q[4]) || !ImageTraits::same_color(p[5], q[5]) ||
              !ImageTraits::same_color(p[6], q[6]) || !ImageTraits::same_color(p[7], q[7]))
            return false;
        }
      }
    }
    else if constexpr (ImageTraits::bytes_per_pixel == 1) {
      for (; x + 16 <= w; x += 16, p += 16, q += 16) {
        __m128i r = _mm_cmpeq_epi8(*(const __m128i*)p, *(const __m128i*)q);
        if (_mm_movemask_epi8(r) != 0xffff) { // !_mm_test_all_ones(r)
          return false;
        }
      }
    }
  #endif
#endif // DOC_USE_ALIGNED_PIXELS
    {
      for (; x + 4 <= w; x += 4, p += 4, q += 4) {
        if (!ImageTraits::same_color(p[0], q[0]) || !ImageTraits::same_color(p[1], q[1]) ||
            !ImageTraits::same_color(p[2], q[2]) || !ImageTraits::same_color(p[3], q[3]))
          return false;
      }
    }

    for (; x < w; ++x, ++p, ++q) {
      if (!ImageTraits::same_color(*p, *q))
        return false;
    }
  }
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
    case IMAGE_TILEMAP:   return is_plain_image_templ<TilemapTraits>(img, c);
  }
  return false;
}

bool is_empty_image(const Image* img)
{
  color_t c = 0; // alpha = 0
  if (img->colorMode() == ColorMode::INDEXED)
    c = img->maskColor();
  return is_plain_image(img, c);
}

int count_diff_between_images(const Image* i1, const Image* i2)
{
  if ((i1->pixelFormat() != i2->pixelFormat()) || (i1->width() != i2->width()) ||
      (i1->height() != i2->height()))
    return -1;

  switch (i1->pixelFormat()) {
    case IMAGE_RGB:       return count_diff_between_images_templ<RgbTraits>(i1, i2);
    case IMAGE_GRAYSCALE: return count_diff_between_images_templ<GrayscaleTraits>(i1, i2);
    case IMAGE_INDEXED:   return count_diff_between_images_templ<IndexedTraits>(i1, i2);
    case IMAGE_BITMAP:    return count_diff_between_images_templ<BitmapTraits>(i1, i2);
    case IMAGE_TILEMAP:   return count_diff_between_images_templ<TilemapTraits>(i1, i2);
  }

  ASSERT(false);
  return -1;
}

bool is_same_image_slow(const Image* i1, const Image* i2)
{
  if ((i1->colorMode() != i2->colorMode()) || (i1->width() != i2->width()) ||
      (i1->height() != i2->height()))
    return false;

  DOC_DISPATCH_BY_COLOR_MODE(i1->colorMode(), is_same_image_templ, i1, i2);

  ASSERT(false);
  return false;
}

bool is_same_image(const Image* i1, const Image* i2)
{
  const ColorMode cm = i1->colorMode();

  if ((cm != i2->colorMode()) || (i1->width() != i2->width()) || (i1->height() != i2->height()))
    return false;

  if (cm == ColorMode::BITMAP)
    return is_same_image_templ<BitmapTraits>(i1, i2);

  DOC_DISPATCH_BY_COLOR_MODE_EXCLUDE_BITMAP(cm, is_same_image_simd_templ, i1, i2);

  ASSERT(false);
  return false;
}

void remap_image(Image* image, const Remap& remap)
{
  ASSERT(image->pixelFormat() == IMAGE_INDEXED || image->pixelFormat() == IMAGE_TILEMAP);

  switch (image->pixelFormat()) {
    case IMAGE_INDEXED:
      transform_image<IndexedTraits>(image, [&remap](color_t c) -> color_t {
        auto to = remap[c];
        if (to != Remap::kUnused)
          return to;
        else
          return c;
      });
      break;
    case IMAGE_TILEMAP:
      transform_image<TilemapTraits>(image, [&remap](color_t c) -> color_t {
        auto to = remap[tile_geti(c)];
        if (c == notile || to == Remap::kNoTile)
          return notile;
        else if (to != Remap::kUnused)
          return tile(to, tile_getf(c));
        else
          return c;
      });
      break;
  }
}

// TODO test this hash routine and find a better alternative

template<typename ImageTraits, uint32_t Mask>
static uint32_t calculate_image_hash_templ(const Image* image, const gfx::Rect& bounds)
{
#if defined(__LP64__) || defined(__x86_64__) || defined(_WIN64)
  #define CITYHASH(buf, len) (CityHash64(buf, len) & 0xffffffff)
  static_assert(sizeof(void*) == 8, "This CPU is not 64-bit");
#else
  #define CITYHASH(buf, len) CityHash32(buf, len)
  static_assert(sizeof(void*) == 4, "This CPU is not 32-bit");
#endif

  const uint32_t widthBytes = ImageTraits::bytes_per_pixel * bounds.w;
  const uint32_t len = widthBytes * bounds.h;
  if (bounds == image->bounds() && widthBytes == image->rowBytes()) {
    return CITYHASH((const char*)image->getPixelAddress(0, 0), len);
  }
  else {
    std::vector<uint8_t> buf(len);
    uint8_t* dst = &buf[0];
    for (int y = 0; y < bounds.h; ++y, dst += widthBytes) {
      auto src = (const uint8_t*)image->getPixelAddress(bounds.x, bounds.y + y);
      std::copy(src, src + widthBytes, dst);
    }
    return CITYHASH((const char*)&buf[0], buf.size());
  }
}

uint32_t calculate_image_hash(const Image* img, const gfx::Rect& bounds)
{
  switch (img->pixelFormat()) {
    case IMAGE_RGB: return calculate_image_hash_templ<RgbTraits, rgba_rgb_mask>(img, bounds);
    case IMAGE_GRAYSCALE:
      return calculate_image_hash_templ<GrayscaleTraits, graya_v_mask>(img, bounds);
    case IMAGE_INDEXED: return calculate_image_hash_templ<IndexedTraits, 0xff>(img, bounds);
    case IMAGE_BITMAP:  return calculate_image_hash_templ<BitmapTraits, 1>(img, bounds);
  }
  ASSERT(false);
  return 0;
}

void preprocess_transparent_pixels(Image* image)
{
  switch (image->pixelFormat()) {
    case IMAGE_RGB: {
      LockImageBits<RgbTraits> bits(image);
      auto it = bits.begin(), end = bits.end();
      for (; it != end; ++it) {
        if (rgba_geta(*it) == 0)
          *it = 0;
      }
      break;
    }

    case IMAGE_GRAYSCALE: {
      LockImageBits<GrayscaleTraits> bits(image);
      auto it = bits.begin(), end = bits.end();
      for (; it != end; ++it) {
        if (graya_geta(*it) == 0)
          *it = 0;
      }
      break;
    }
  }
}

} // namespace doc
