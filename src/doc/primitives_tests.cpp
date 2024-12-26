// Aseprite Document Library
// Copyright (c) 2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/primitives.h"

#include "doc/algorithm/random_image.h"
#include "doc/image_impl.h"
#include "doc/image_ref.h"
#include "doc/primitives_fast.h"

#include <random>

// #define FULL_TEST 1

using namespace doc;
using namespace gfx;

template<typename T>
class Primitives : public testing::Test {
protected:
  Primitives() {}
};

using ImageAllTraits =
  testing::Types<RgbTraits, GrayscaleTraits, IndexedTraits, BitmapTraits, TilemapTraits>;
TYPED_TEST_SUITE(Primitives, ImageAllTraits);

TYPED_TEST(Primitives, IsSameImage)
{
  using ImageTraits = TypeParam;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0, 256);

#if FULL_TEST
  int w = 200;
  int h = 200;
  {
    {
#else
  for (int h = 2; h < 207; h += 5) {
    for (int w = 2; w < 207; w += 5) {
#endif
      ImageRef a(Image::create(ImageTraits::pixel_format, w, h));
      doc::algorithm::random_image(a.get());

      ImageRef b(Image::createCopy(a.get()));
#if FULL_TEST
      for (int v = 0; v < h; ++v)
        for (int u = 0; u < w; ++u) {
#else
      for (int i = 0; i < 32; ++i) {
        int u = dist(gen) % w;
        int v = dist(gen) % h;
#endif
          ASSERT_TRUE(is_same_image_slow(a.get(), b.get()));
          ASSERT_TRUE(is_same_image(a.get(), b.get()));

          auto old = get_pixel_fast<ImageTraits>(b.get(), u, v);
          if (old != 0)
            put_pixel_fast<ImageTraits>(b.get(), u, v, 0);
          else
            put_pixel_fast<ImageTraits>(b.get(), u, v, 1);

          ASSERT_FALSE(is_same_image_slow(a.get(), b.get()));
          ASSERT_FALSE(is_same_image(a.get(), b.get()));

          put_pixel_fast<ImageTraits>(b.get(), u, v, old);
        }
    }
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
