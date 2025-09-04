// Aseprite Document Library
// Copyright (c) 2018-2025 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/algorithm/random_image.h"
#include "doc/image.h"
#include "doc/primitives.h"

#include <memory>

using namespace base;
using namespace doc;

template<typename T>
class ImageAllTypes : public testing::Test {
protected:
  ImageAllTypes() {}
};

using ImageAllTraits = testing::Types<RgbTraits, GrayscaleTraits, IndexedTraits, BitmapTraits>;

TYPED_TEST_SUITE(ImageAllTypes, ImageAllTraits);

#if DOC_USE_BITMAP_AS_1BPP

template<typename T>
class ImageAllTypesNoBitmap : public testing::Test {
protected:
  ImageAllTypesNoBitmap() {}
};

using ImageAllTraitsNoBitmap = testing::Types<RgbTraits, GrayscaleTraits, IndexedTraits>;
TYPED_TEST_SUITE(ImageAllTypesNoBitmap, ImageAllTraitsNoBitmap);

#else // !DOC_USE_BITMAP_AS_1BPP

  #define ImageAllTypesNoBitmap ImageAllTypes

#endif // !DOC_USE_BITMAP_AS_1BPP

TYPED_TEST(ImageAllTypes, PutGetAndIterators)
{
  using ImageTraits = TypeParam;

  std::vector<int> lengths = { 1, 4, 7, 8, 9, 15, 33 };
  std::vector<gfx::Size> sizes(lengths.size() * lengths.size());
  std::size_t k = 0;
  for (std::size_t i = 0; i < lengths.size(); ++i)
    for (std::size_t j = 0; j < lengths.size(); ++j)
      sizes[k++] = gfx::Size(lengths[j], lengths[i]);

  for (const auto& size : sizes) {
    const int w = size.w;
    const int h = size.h;
    std::unique_ptr<Image> image(Image::create(ImageTraits::pixel_format, w, h));
    std::vector<int> data(w * h);

    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
        data[y * w + x] = (std::rand() % ImageTraits::max_value);

    for (int i = 0; i < w * h; ++i)
      put_pixel(image.get(), i % w, i / w, data[i]);

    for (int i = 0; i < w * h; ++i)
      ASSERT_EQ(data[i], get_pixel(image.get(), i % w, i / w));

    std::vector<gfx::Rect> areas;

    // Read-only iterator (whole image)
    {
      const LockImageBits<ImageTraits> bits((const Image*)image.get());
      auto begin = bits.begin(), it = begin, end = bits.end();

      for (int i = 0; it != end; ++it, ++i) {
        assert(data[i] == *it);
        ASSERT_EQ(data[i], *it);
      }
    }

    // Read-only iterator (areas)
    for (int i = 0;; ++i) {
      gfx::Rect bounds(i, i, w - i * 2, h - i * 2);
      if (bounds.w <= 0 || bounds.h <= 0)
        break;

      const LockImageBits<ImageTraits> bits((const Image*)image.get(), bounds);
      auto begin = bits.begin(), it = begin, end = bits.end();

      for (int y = bounds.y; y < bounds.y + bounds.h; ++y) {
        for (int x = bounds.x; x < bounds.x + bounds.w; ++x, ++it) {
          SCOPED_TRACE(x);
          SCOPED_TRACE(y);

          ASSERT_TRUE(it != end);
          EXPECT_EQ(data[y * w + x], *it);
        }
      }

      EXPECT_TRUE(it == end);
    }

    // Write iterator (whole image)
    {
      LockImageBits<ImageTraits> bits(image.get(), Image::WriteLock);
      auto begin = bits.begin(), it = begin, end = bits.end();

      for (int i = 0; it != end; ++it, ++i) {
        *it = 1;
        EXPECT_EQ(1, *it);
      }

      it = begin;
      for (int i = 0; it != end; ++it, ++i) {
        EXPECT_EQ(1, *it);
      }
    }
  }
}

TEST(Image, DiffRgbImages)
{
  std::unique_ptr<Image> a(Image::create(IMAGE_RGB, 32, 32));
  std::unique_ptr<Image> b(Image::create(IMAGE_RGB, 32, 32));

  clear_image(a.get(), rgba(0, 0, 0, 0));
  clear_image(b.get(), rgba(0, 0, 0, 0));

  ASSERT_EQ(0, count_diff_between_images(a.get(), b.get()));
  ASSERT_TRUE(is_same_image(a.get(), b.get()));

  // No difference because alpha=0
  put_pixel(a.get(), 0, 0, rgba(255, 0, 0, 0));
  ASSERT_EQ(0, count_diff_between_images(a.get(), b.get()));
  ASSERT_TRUE(is_same_image(a.get(), b.get()));

  put_pixel(a.get(), 0, 0, rgba(255, 0, 0, 255));
  ASSERT_EQ(1, count_diff_between_images(a.get(), b.get()));
  ASSERT_FALSE(is_same_image(a.get(), b.get()));

  put_pixel(a.get(), 1, 1, rgba(0, 0, 255, 128));
  ASSERT_EQ(2, count_diff_between_images(a.get(), b.get()));
  ASSERT_FALSE(is_same_image(a.get(), b.get()));
}

TYPED_TEST(ImageAllTypes, DrawHLine)
{
  using ImageTraits = TypeParam;

  std::vector<int> lengths = { 7, 8, 9, 15, 16, 17, 31, 32, 33 };
  std::vector<gfx::Size> sizes(lengths.size() * lengths.size());
  std::size_t k = 0;
  for (std::size_t i = 0; i < lengths.size(); ++i)
    for (std::size_t j = 0; j < lengths.size(); ++j)
      sizes[k++] = gfx::Size(lengths[j], lengths[i]);

  for (const auto& size : sizes) {
    const int w = size.w;
    const int h = size.h;
    std::unique_ptr<Image> image(Image::create(ImageTraits::pixel_format, w, h));
    image->clear(0);

    for (int c = 0; c < 100; ++c) {
      int x = rand() % w;
      int y = rand() % h;
      int x2 = x + (rand() % (w - x));
      image->drawHLine(x, y, x2, rand() % ImageTraits::max_value);
    }
  }
}

TYPED_TEST(ImageAllTypes, FillRect)
{
  using ImageTraits = TypeParam;

  for (int i = 0; i <= 110; i += 11) {
    const int w = 1 + i;
    const int h = 1 + i;

    std::unique_ptr<Image> image(Image::create(ImageTraits::pixel_format, w, h));
    color_t color = (rand() % ImageTraits::max_value);
    if (!color)
      color = 1;

    for (int j = 0; j <= 1100; j += 11) {
      const int x1 = rand() % w;
      const int y1 = rand() % h;
      const int x2 = x1 + (rand() % (w - x1));
      const int y2 = y1 + (rand() % (h - y1));

      image->clear(0);
      fill_rect(image.get(), x1, y1, x2, y2, color);

      // Check
      for (int v = 0; v < h; ++v) {
        for (int u = 0; u < w; ++u) {
          color_t pixel = get_pixel_fast<ImageTraits>(image.get(), u, v);
          if (u >= x1 && v >= y1 && u <= x2 && v <= y2)
            EXPECT_EQ(color, pixel);
          else
            EXPECT_EQ(0, pixel);
        }
      }
    }
  }
}

TYPED_TEST(ImageAllTypesNoBitmap, NewIterators)
{
  using ImageTraits = TypeParam;

  for (int i = 0; i < 100; ++i) {
    const int w = 1 + i;
    const int h = 1 + i;

    std::unique_ptr<Image> image(Image::create(ImageTraits::pixel_format, w, h));
    doc::algorithm::random_image(image.get());

    // TopLeft
    {
      int v = 0;
      auto it = image->readArea(image->bounds(), IteratorStart::TopLeft);
      while (it.nextLine()) {
        auto* addr = (typename ImageTraits::address_t)it.addr8();
        for (int u = 0; u < w; ++u, ++addr) {
          auto expected = get_pixel_fast<ImageTraits>(image.get(), u, v);
          ASSERT_EQ(expected, *addr);
        }
        ++v;
      }
    }

    // TopRight
    {
      int v = 0;
      auto it = image->readArea(image->bounds(), IteratorStart::TopRight);
      while (it.nextLine()) {
        auto* addr = (typename ImageTraits::address_t)it.addr8();
        for (int u = w - 1; u >= 0; --u, --addr) {
          auto expected = get_pixel_fast<ImageTraits>(image.get(), u, v);
          ASSERT_EQ(expected, *addr);
        }
        ++v;
      }
    }

    // BottomLeft
    {
      int v = h - 1;
      auto it = image->readArea(image->bounds(), IteratorStart::BottomLeft);
      while (it.nextLine()) {
        auto* addr = (typename ImageTraits::address_t)it.addr8();
        for (int u = 0; u < w; ++u, ++addr) {
          auto expected = get_pixel_fast<ImageTraits>(image.get(), u, v);
          ASSERT_EQ(expected, *addr);
        }
        --v;
      }
    }

    // BottomRight
    {
      int v = h - 1;
      auto it = image->readArea(image->bounds(), IteratorStart::BottomRight);
      while (it.nextLine()) {
        auto* addr = (typename ImageTraits::address_t)it.addr8();
        for (int u = w - 1; u >= 0; --u, --addr) {
          auto expected = get_pixel_fast<ImageTraits>(image.get(), u, v);
          ASSERT_EQ(expected, *addr);
        }
        --v;
      }
    }
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
