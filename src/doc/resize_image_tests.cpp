// Aseprite Document Library
// Copyright (c) 2022 Igara Studio S.A.
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

// Base image
color_t test_image_base_3x3[9] = { 0x000000, 0xffffff, 0x000000, 0xffffff, 0xffffff,
                                   0xffffff, 0x000000, 0xffffff, 0x000000 };

// Base image scaled to 9x9 with nearest neighbor interpolation
color_t test_image_scaled_9x9_nearest[81] = {
  0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000,
  0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
  0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
  0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
  0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000, 0x000000
};

// Base image scalled to 9x9 with bilinear interpolation
color_t test_image_scaled_9x9_bilinear[81] = {
  0x000000, 0x000000, 0x565656, 0xa9a9a9, 0xffffff, 0xa9a9a9, 0x565656, 0x000000, 0x000000,
  0x000000, 0x000000, 0x565656, 0xa9a9a9, 0xffffff, 0xa9a9a9, 0x565656, 0x000000, 0x000000,
  0x565656, 0x565656, 0x8f8f8f, 0xc6c6c6, 0xffffff, 0xc6c6c6, 0x8f8f8f, 0x565656, 0x565656,
  0xa9a9a9, 0xa9a9a9, 0xc6c6c6, 0xe2e2e2, 0xffffff, 0xe2e2e2, 0xc6c6c6, 0xa9a9a9, 0xa9a9a9,
  0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
  0xa9a9a9, 0xa9a9a9, 0xc6c6c6, 0xe2e2e2, 0xffffff, 0xe2e2e2, 0xc6c6c6, 0xa9a9a9, 0xa9a9a9,
  0x565656, 0x565656, 0x8f8f8f, 0xc6c6c6, 0xffffff, 0xc6c6c6, 0x8f8f8f, 0x565656, 0x565656,
  0x000000, 0x000000, 0x565656, 0xa9a9a9, 0xffffff, 0xa9a9a9, 0x565656, 0x000000, 0x000000,
  0x000000, 0x000000, 0x565656, 0xa9a9a9, 0xffffff, 0xa9a9a9, 0x565656, 0x000000, 0x000000
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
  algorithm::resize_image(src.get(),
                          dst.get(),
                          algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR,
                          nullptr,
                          nullptr,
                          -1);

  ASSERT_EQ(0, count_diff_between_images(dst.get(), dst_expected.get()));

  ImageRef dst2(Image::create(IMAGE_RGB, 3, 3));
  algorithm::resize_image(dst.get(),
                          dst2.get(),
                          algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR,
                          nullptr,
                          nullptr,
                          -1);
  ASSERT_EQ(0, count_diff_between_images(src.get(), dst2.get()));
}

#if 0 // TODO complete this test
TEST(ResizeImage, BilinearInterpRGBType)
{
  ImageRef src(create_image_from_data(IMAGE_RGB, test_image_base_3x3, 3, 3));
  ImageRef dst_expected(create_image_from_data(IMAGE_RGB, test_image_scaled_9x9_bilinear, 9, 9));

  ImageRef dst(Image::create(IMAGE_RGB, 9, 9));
  algorithm::resize_image(src.get(), dst.get(),
                          algorithm::RESIZE_METHOD_BILINEAR,
                          nullptr, nullptr, -1);

  ASSERT_EQ(0, count_diff_between_images(dst.get(), dst_expected.get()));
}
#endif

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
