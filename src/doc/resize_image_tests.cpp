// Aseprite Document Library
// Copyright (c) 2022-present Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/algorithm/resize_image.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"

using namespace std;
using namespace doc;

// Test data

// Base image (white opaque cross with transparent corners)
color_t test_image_base_3x3[9] = { 0x00000000, 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff,
                                   0xffffffff, 0x00000000, 0xffffffff, 0x00000000 };

// Base image scaled to 9x9 with nearest neighbor interpolation
color_t test_image_scaled_9x9_nearest[81] = {
  0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff,
  0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000,
  0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000,
  0x00000000,
};

// Base image scaled to 9x9 with bilinear interpolation
color_t test_image_scaled_9x9_bilinear[81] = {
  0x00000000, 0x3fffffff, 0x7fffffff, 0xbfffffff, 0xffffffff, 0xbfffffff, 0x7fffffff, 0x3fffffff,
  0x00000000, 0x3fffffff, 0x6fffffff, 0x9fffffff, 0xcfffffff, 0xffffffff, 0xcfffffff, 0x9fffffff,
  0x6fffffff, 0x3fffffff, 0x7fffffff, 0x9fffffff, 0xbfffffff, 0xdfffffff, 0xffffffff, 0xdfffffff,
  0xbfffffff, 0x9fffffff, 0x7fffffff, 0xbfffffff, 0xcfffffff, 0xdfffffff, 0xefffffff, 0xffffffff,
  0xefffffff, 0xdfffffff, 0xcfffffff, 0xbfffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xbfffffff, 0xcfffffff, 0xdfffffff,
  0xefffffff, 0xffffffff, 0xefffffff, 0xdfffffff, 0xcfffffff, 0xbfffffff, 0x7fffffff, 0x9fffffff,
  0xbfffffff, 0xdfffffff, 0xffffffff, 0xdfffffff, 0xbfffffff, 0x9fffffff, 0x7fffffff, 0x3fffffff,
  0x6fffffff, 0x9fffffff, 0xcfffffff, 0xffffffff, 0xcfffffff, 0x9fffffff, 0x6fffffff, 0x3fffffff,
  0x00000000, 0x3fffffff, 0x7fffffff, 0xbfffffff, 0xffffffff, 0xbfffffff, 0x7fffffff, 0x3fffffff,
  0x00000000,
};

ImageRef create_image_from_data(PixelFormat format, color_t* data, int width, int height)
{
  ImageRef new_image(Image::create(format, width, height));
  for (int i = 0; i < width * height; i++) {
    new_image->putPixel(i % width, i / width, data[i]);
  }
  return new_image;
}

TEST(ResizeImage, NearestNeighborInterp)
{
  ImageRef src(create_image_from_data(IMAGE_RGB, test_image_base_3x3, 3, 3));
  ImageRef dst_expected(create_image_from_data(IMAGE_RGB, test_image_scaled_9x9_nearest, 9, 9));
  ImageRef dst(Image::create(IMAGE_RGB, 9, 9));

  algorithm::ResizeImage resize;
  resize.method = algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR;
  resize(src.get(), dst.get());

  ASSERT_EQ(0, count_diff_between_images(dst.get(), dst_expected.get()));

  ImageRef dst2(Image::create(IMAGE_RGB, 3, 3));
  resize(dst.get(), dst2.get());
  ASSERT_EQ(0, count_diff_between_images(src.get(), dst2.get()));
}

TEST(ResizeImage, BilinearInterpRGBType)
{
  ImageRef src(create_image_from_data(IMAGE_RGB, test_image_base_3x3, 3, 3));
  ImageRef dst_expected(create_image_from_data(IMAGE_RGB, test_image_scaled_9x9_bilinear, 9, 9));
  ImageRef dst(Image::create(IMAGE_RGB, 9, 9));

  algorithm::ResizeImage resize;
  resize.method = algorithm::RESIZE_METHOD_BILINEAR;
  resize(src.get(), dst.get());

  // 'count_diff_between_images_exact_match' is needed here because it detects
  // RGB values even when pixel's alpha is 0. Do not use 'count_diff_between_images'.
  ASSERT_EQ(0, count_diff_between_images_exact_match(dst.get(), dst_expected.get()));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
