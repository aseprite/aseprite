// Aseprite Document Library
// Copyright (c) 2018 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/image_impl.h"
#include "doc/primitives.h"

#include <memory>

using namespace base;
using namespace doc;

template<typename T>
class ImageAllTypes : public testing::Test {
protected:
  ImageAllTypes() { }
};

typedef testing::Types<RgbTraits, GrayscaleTraits, IndexedTraits, BitmapTraits> ImageAllTraits;
TYPED_TEST_CASE(ImageAllTypes, ImageAllTraits);

TYPED_TEST(ImageAllTypes, PutGetAndIterators)
{
  typedef TypeParam ImageTraits;

  std::vector<int> lengths;
  lengths.push_back(1);
  lengths.push_back(4);
  lengths.push_back(7);
  lengths.push_back(8);
  lengths.push_back(9);
  lengths.push_back(15);
  lengths.push_back(16);
  lengths.push_back(17);
  lengths.push_back(31);
  lengths.push_back(32);
  lengths.push_back(33);

  std::vector<gfx::Size> sizes;
  for (std::size_t i=0; i<lengths.size(); ++i)
    for (std::size_t j=0; j<lengths.size(); ++j)
      sizes.push_back(gfx::Size(lengths[j], lengths[i]));

  for (std::vector<gfx::Size>::iterator sizes_it=sizes.begin(); sizes_it!=sizes.end(); ++sizes_it) {
    int w = sizes_it->w;
    int h = sizes_it->h;
    std::unique_ptr<Image> image(Image::create(ImageTraits::pixel_format, w, h));
    std::vector<int> data(w*h);

    for (int y=0; y<h; ++y)
      for (int x=0; x<w; ++x)
        data[y*w+x] = (std::rand() % ImageTraits::max_value);

    for (int i=0; i<w*h; ++i)
      put_pixel(image.get(), i%w, i/w, data[i]);

    for (int i=0; i<w*h; ++i)
      ASSERT_EQ(data[i], get_pixel(image.get(), i%w, i/w));

    std::vector<gfx::Rect> areas;

    // Read-only iterator (whole image)
    {
      const LockImageBits<ImageTraits> bits((const Image*)image.get());
      typename LockImageBits<ImageTraits>::const_iterator
        begin = bits.begin(),
        it = begin,
        end = bits.end();

      for (int i=0; it != end; ++it, ++i) {
        assert(data[i] == *it);
        ASSERT_EQ(data[i], *it);
      }
    }

    // Read-only iterator (areas)
    for (int i=0; ; ++i) {
      gfx::Rect bounds(i, i, w-i*2, h-i*2);
      if (bounds.w <= 0 || bounds.h <= 0)
        break;

      const LockImageBits<ImageTraits> bits((const Image*)image.get(), bounds);
      typename LockImageBits<ImageTraits>::const_iterator
        begin = bits.begin(),
        it = begin,
        end = bits.end();

      for (int y=bounds.y; y<bounds.y+bounds.h; ++y) {
        for (int x=bounds.x; x<bounds.x+bounds.w; ++x, ++it) {
          SCOPED_TRACE(x);
          SCOPED_TRACE(y);

          ASSERT_TRUE(it != end);
          EXPECT_EQ(data[y*w+x], *it);
        }
      }

      EXPECT_TRUE(it == end);
    }

    // Write iterator (whole image)
    {
      LockImageBits<ImageTraits> bits(image.get(), Image::WriteLock);
      typename LockImageBits<ImageTraits>::iterator
        begin = bits.begin(),
        it = begin,
        end = bits.end();

      for (int i=0; it != end; ++it, ++i) {
        *it = 1;
        EXPECT_EQ(1, *it);
      }

      it = begin;
      for (int i=0; it != end; ++it, ++i) {
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
  typedef TypeParam ImageTraits;

  std::vector<int> lengths;
  lengths.push_back(7);
  lengths.push_back(8);
  lengths.push_back(9);
  lengths.push_back(15);
  lengths.push_back(16);
  lengths.push_back(17);
  lengths.push_back(31);
  lengths.push_back(32);
  lengths.push_back(33);

  std::vector<gfx::Size> sizes;
  for (std::size_t i=0; i<lengths.size(); ++i)
    for (std::size_t j=0; j<lengths.size(); ++j)
      sizes.push_back(gfx::Size(lengths[j], lengths[i]));

  for (std::vector<gfx::Size>::iterator sizes_it=sizes.begin(); sizes_it!=sizes.end(); ++sizes_it) {
    int w = sizes_it->w;
    int h = sizes_it->h;
    std::unique_ptr<Image> image(Image::create(ImageTraits::pixel_format, w, h));
    image->clear(0);

    for (int c=0; c<100; ++c) {
      int x = rand() % w;
      int y = rand() % h;
      int x2 = x + (rand() % (w-x));
      image->drawHLine(x, y, x2, rand() % ImageTraits::max_value);
    }
  }
}

TYPED_TEST(ImageAllTypes, FillRect)
{
  typedef TypeParam ImageTraits;

  for (int i=0; i<100; ++i) {
    int w = 1+i;
    int h = 1+i;

    std::unique_ptr<Image> image(Image::create(ImageTraits::pixel_format, w, h));
    color_t color = (rand() % ImageTraits::max_value);
    if (!color)
      color = 1;

    for (int j=0; j<1000; ++j) {
      int x1 = rand() % w;
      int y1 = rand() % h;
      int x2 = x1 + (rand() % (w-x1));
      int y2 = y1 + (rand() % (h-y1));

      image->clear(0);
      fill_rect(image.get(), x1, y1, x2, y2, color);

      // Check
      for (int v=0; v<h; ++v) {
        for (int u=0; u<w; ++u) {
          color_t pixel = get_pixel_fast<ImageTraits>(image.get(), u, v);
          if (u >= x1 && v >= y1 &&
              u <= x2 && v <= y2)
            EXPECT_EQ(color, pixel);
          else
            EXPECT_EQ(0, pixel);
        }
      }
    }
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
