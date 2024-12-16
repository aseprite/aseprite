// Aseprite Document Library
// Copyright (c) 2020 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gtest/gtest.h"

#include "doc/algorithm/fill_selection.h"
#include "doc/grid.h"
#include "doc/image.h"
#include "doc/mask.h"

using namespace doc;
using namespace gfx;

::testing::AssertionResult cmp_img(const std::vector<color_t>& pixels, const Image* image)
{
  int c = 0;
  for (int y = 0; y < image->height(); ++y) {
    for (int x = 0; x < image->width(); ++x) {
      if (pixels[c] != image->getPixel(x, y)) {
        return ::testing::AssertionFailure()
               << "ExpectedPixel=" << (int)pixels[c]
               << " ActualPixel=" << (int)image->getPixel(x, y) << " x=" << x << " y=" << y;
      }
      ++c;
    }
  }
  return ::testing::AssertionSuccess();
}

TEST(FillSelection, Image)
{
  ImageRef image(Image::create(IMAGE_INDEXED, 4, 4));
  image->clear(1);

  // No-op (no intersection between image & mask)
  Mask mask;
  mask.replace(Rect(0, 0, 1, 5));
  algorithm::fill_selection(image.get(), Rect(1, 1, 4, 4), &mask, 2, nullptr);
  EXPECT_TRUE(cmp_img({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, image.get()));

  mask.replace(Rect(1, 0, 2, 3));
  algorithm::fill_selection(image.get(), Rect(1, 1, 4, 4), &mask, 2, nullptr);
  EXPECT_TRUE(cmp_img({ 2, 2, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, image.get()));

  mask.replace(Rect(2, 2, 2, 3));
  algorithm::fill_selection(image.get(), Rect(1, 3, 4, 4), &mask, 3, nullptr);
  EXPECT_TRUE(cmp_img({ 2, 3, 3, 1, 2, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, image.get()));
}

TEST(FillSelection, Tilemap)
{
  ImageRef image(Image::create(IMAGE_TILEMAP, 4, 4));
  image->clear(1);

  Grid grid(Size(8, 8));
  grid.origin(Point(4, 4));

  // No-op (no intersection between image & mask)
  Mask mask;
  mask.replace(Rect(0, 0, 4, 4));
  algorithm::fill_selection(image.get(), Rect(4, 4, 32, 32), &mask, 2, &grid);
  EXPECT_TRUE(cmp_img({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, image.get()));

  mask.replace(Rect(0, 0, 5, 5));
  algorithm::fill_selection(image.get(), Rect(4, 4, 32, 32), &mask, 2, &grid);
  EXPECT_TRUE(cmp_img({ 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, image.get()));

  mask.replace(Rect(12, 12, 9, 8));
  algorithm::fill_selection(image.get(), Rect(4, 4, 32, 32), &mask, 3, &grid);
  EXPECT_TRUE(cmp_img({ 2, 1, 1, 1, 1, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, image.get()));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
