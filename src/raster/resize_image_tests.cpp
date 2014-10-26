/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include <gtest/gtest.h>

#include "raster/algorithm/resize_image.h"
#include "raster/color.h"
#include "raster/image.h"
#include "raster/primitives.h"

using namespace std;
using namespace raster;

// Test data

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

TEST(ResizeImage, NearestNeighborInterp)
{
  Image* src = create_image_from_data(IMAGE_RGB, test_image_base_3x3, 3, 3);
  Image* dst_expected = create_image_from_data(IMAGE_RGB, test_image_scaled_9x9_nearest, 9, 9);

  Image* dst = Image::create(IMAGE_RGB, 9, 9);
  algorithm::resize_image(src, dst, algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR, NULL, NULL);

  ASSERT_EQ(0, count_diff_between_images(dst, dst_expected));
}

#if 0                           // TODO complete this test
TEST(ResizeImage, BilinearInterpRGBType)
{
  Image* src = create_image_from_data(IMAGE_RGB, test_image_base_3x3, 3, 3);
  Image* dst_expected = create_image_from_data(IMAGE_RGB, test_image_scaled_9x9_bilinear, 9, 9);

  Image* dst = Image::create(IMAGE_RGB, 9, 9);
  algorithm::resize_image(src, dst, algorithm::RESIZE_METHOD_BILINEAR, NULL, NULL);

  ASSERT_EQ(0, count_diff_between_images(dst, dst_expected));
}
#endif

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
