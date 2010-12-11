// ASE gfx library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "gfx/hsv.h"
#include "gfx/rgb.h"

using namespace gfx;
using namespace std;

ostream& operator<<(ostream& os, const Hsv& hsv)
{
  return os << "("
	    << hsv.hueInt() << ", "
	    << hsv.saturationInt() << ", "
	    << hsv.valueInt() << "); real: ("
	    << hsv.hue() << ", "
	    << hsv.saturation() << ", "
	    << hsv.value() << ")";
}

TEST(Hsv, FromRgb)
{
  EXPECT_EQ(Hsv(  0.0, 0.000, 0.000), Hsv(Rgb(  0,   0,   0)));
  EXPECT_EQ(Hsv(  0.0, 1.000, 0.010), Hsv(Rgb(  3,   0,   0)));
  EXPECT_EQ(Hsv(  0.0, 1.000, 0.990), Hsv(Rgb(252,   0,   0)));
  EXPECT_EQ(Hsv(  0.0, 1.000, 1.000), Hsv(Rgb(255,   0,   0)));
  EXPECT_EQ(Hsv( 60.0, 1.000, 0.750), Hsv(Rgb(191, 191,   0)));
  EXPECT_EQ(Hsv(120.0, 1.000, 0.500), Hsv(Rgb(  0, 128,   0)));
  EXPECT_EQ(Hsv(120.0, 1.000, 1.000), Hsv(Rgb(  0, 255,   0)));
  EXPECT_EQ(Hsv(180.0, 0.500, 1.000), Hsv(Rgb(128, 255, 255)));
  EXPECT_EQ(Hsv(240.0, 0.500, 1.000), Hsv(Rgb(128, 128, 255)));
  EXPECT_EQ(Hsv(240.0, 1.000, 1.000), Hsv(Rgb(  0,   0, 255)));
  EXPECT_EQ(Hsv(300.0, 0.667, 0.750), Hsv(Rgb(191,  64, 191)));
  EXPECT_EQ(Hsv(360.0, 1.000, 0.990), Hsv(Rgb(252,   0,   0)));
  EXPECT_EQ(Hsv(360.0, 1.000, 1.000), Hsv(Rgb(255,   0,   0)));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
