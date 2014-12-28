// Aseprite Gfx Library
// Copyright (C) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "gfx/rgb.h"
#include "gfx/hsv.h"

using namespace gfx;
using namespace std;

namespace gfx {

  ostream& operator<<(ostream& os, const Rgb& rgb) {
    return os << "("
              << rgb.red() << ", "
              << rgb.green() << ", "
              << rgb.blue() << ")";
  }
  
}

TEST(Rgb, Ctor)
{
  EXPECT_EQ(1, Rgb(1, 2, 3).red());
  EXPECT_EQ(2, Rgb(1, 2, 3).green());
  EXPECT_EQ(3, Rgb(1, 2, 3).blue());
  EXPECT_EQ(Rgb(0, 0, 0), Rgb());
}

TEST(Rgb, Equal)
{
  EXPECT_TRUE(Rgb(1, 2, 3) == Rgb(1, 2, 3));
  EXPECT_FALSE(Rgb(1, 2, 3) != Rgb(1, 2, 3));
  EXPECT_FALSE(Rgb(1, 2, 3) == Rgb(1, 3, 2));
  EXPECT_FALSE(Rgb(0, 0, 0) == Rgb(0, 0, 1));
  EXPECT_FALSE(Rgb(0, 0, 0) == Rgb(0, 1, 0));
  EXPECT_FALSE(Rgb(0, 0, 0) == Rgb(1, 0, 0));
}

TEST(Rgb, MaxComponent)
{
  EXPECT_EQ(3, Rgb(1, 2, 3).maxComponent());
  EXPECT_EQ(3, Rgb(1, 3, 2).maxComponent());
  EXPECT_EQ(3, Rgb(2, 1, 3).maxComponent());
  EXPECT_EQ(3, Rgb(2, 3, 1).maxComponent());
  EXPECT_EQ(3, Rgb(3, 1, 2).maxComponent());
  EXPECT_EQ(3, Rgb(3, 2, 1).maxComponent());
}

TEST(Rgb, MinComponent)
{
  EXPECT_EQ(1, Rgb(1, 2, 3).minComponent());
  EXPECT_EQ(1, Rgb(1, 3, 2).minComponent());
  EXPECT_EQ(1, Rgb(2, 1, 3).minComponent());
  EXPECT_EQ(1, Rgb(2, 3, 1).minComponent());
  EXPECT_EQ(1, Rgb(3, 1, 2).minComponent());
  EXPECT_EQ(1, Rgb(3, 2, 1).minComponent());
}

TEST(Rgb, FromHsv)
{
  for (double hue = 0.0; hue <= 360.0; hue += 10.0) {
    EXPECT_EQ(Rgb(255, 255, 255), Rgb(Hsv(hue, 0.000, 1.000)));
    EXPECT_EQ(Rgb(128, 128, 128), Rgb(Hsv(hue, 0.000, 0.500)));
    EXPECT_EQ(Rgb(  0,   0,   0), Rgb(Hsv(hue, 0.000, 0.000)));
  }

  EXPECT_EQ(Rgb(  3,   0,   0), Rgb(Hsv(  0.0, 1.000, 0.010)));
  EXPECT_EQ(Rgb(252,   0,   0), Rgb(Hsv(  0.0, 1.000, 0.990)));
  EXPECT_EQ(Rgb(255,   0,   0), Rgb(Hsv(  0.0, 1.000, 1.000)));
  EXPECT_EQ(Rgb(191, 191,   0), Rgb(Hsv( 60.0, 1.000, 0.750)));
  EXPECT_EQ(Rgb(  0, 128,   0), Rgb(Hsv(120.0, 1.000, 0.500)));
  EXPECT_EQ(Rgb(  0, 255,   0), Rgb(Hsv(120.0, 1.000, 1.000)));
  EXPECT_EQ(Rgb(128, 255, 255), Rgb(Hsv(180.0, 0.500, 1.000)));
  EXPECT_EQ(Rgb(128, 128, 255), Rgb(Hsv(240.0, 0.500, 1.000)));
  EXPECT_EQ(Rgb(  0,   0, 255), Rgb(Hsv(240.0, 1.000, 1.000)));
  EXPECT_EQ(Rgb(191,  64, 191), Rgb(Hsv(300.0, 0.667, 0.750)));
  EXPECT_EQ(Rgb(252,   0,   0), Rgb(Hsv(360.0, 1.000, 0.990)));
  EXPECT_EQ(Rgb(255,   0,   0), Rgb(Hsv(360.0, 1.000, 1.000)));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
