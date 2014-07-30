// Aseprite Gfx Library
// Copyright (C) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "raster/color.h"
#include "raster/image.h"
#include "raster/algorithm/resize_image.h"

using namespace std;
using namespace raster;

/***************************
 * Test dat
 */

// Base image
color_t test_image_base_3x3[9] =
{
  0x000000, 0xffffff, 0x000000,
  0xffffff, 0xffffff, 0xffffff,
  0x000000, 0xffffff, 0x000000
};

// Base image scaled to 9x9 with nearest neighbor interpolation
color_t test_image_scaled_9x9_nearest[81] =
{
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
color_t test_image_scaled_9x9_bilinear[81] =
{
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

Image* create_image_from_data(PixelFormat format, color_t* data, int width, int height) 
{
  Image* new_image = Image::create(format, width, height);
  for (int i = 0; i < width * height; i++) {
    new_image->putPixel(i % width, i / width, data[i]);
  }

  return new_image;
}

// Simple pixel to pixel image comparison
bool compare_images(Image* a, Image* b)
{
  for (int y = 0; y < a->height(); y++) {
    for (int x = 0; x < a->width(); x++) {
      if (!(a->getPixel(x, y) == b->getPixel(x, y)))
        return false;
    }
  }
  return true;
}

TEST(ResizeImage, NearestNeighborInterp)
{
  Image* src = create_image_from_data(IMAGE_RGB, test_image_base_3x3, 3, 3);
  Image* dst = Image::create(IMAGE_RGB, 9, 9);

  // Pre-rendered test image for comparison
  Image* test_dst = create_image_from_data(IMAGE_RGB, test_image_scaled_9x9_nearest, 9, 9);

  algorithm::resize_image(src, dst, algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR, NULL, NULL);

  ASSERT_TRUE(compare_images(dst, test_dst)) << "resize_image() result does not match test image!";
}

#if 0                           // TODO complete this test
TEST(ResizeImage, BilinearInterpRGBType)
{
  Image* src = create_image_from_data(IMAGE_RGB, test_image_base_3x3, 3, 3);
  Image* dst = Image::create(IMAGE_RGB, 9, 9);

  // Pre-rendered test image for comparison
  Image* test_dst = create_image_from_data(IMAGE_RGB, test_image_scaled_9x9_bilinear, 9, 9);

  algorithm::resize_image(src, dst, algorithm::RESIZE_METHOD_BILINEAR, NULL, NULL);

  ASSERT_TRUE(compare_images(dst, test_dst)) << "resize_image() result does not match test image!";
}
#endif

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
