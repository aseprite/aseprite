// Aseprite Gfx Library
// Copyright (C) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "gfx/hsv.h"
#include "gfx/rgb.h"

using namespace gfx;
using namespace std;

namespace gfx {

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
  
}

TEST(Hsv, Ctor)
{
  EXPECT_EQ(35.0, Hsv(35.0, 0.50, 0.75).hue());
  EXPECT_EQ(0.50, Hsv(35.0, 0.50, 0.75).saturation());
  EXPECT_EQ(0.75, Hsv(35.0, 0.50, 0.75).value());
  EXPECT_EQ(35, Hsv(35.0, 0.50, 0.75).hueInt());
  EXPECT_EQ(50, Hsv(35.0, 0.50, 0.75).saturationInt());
  EXPECT_EQ(75, Hsv(35.0, 0.50, 0.75).valueInt());
  EXPECT_EQ(Hsv(0, 0, 0), Hsv());
}

TEST(Hsv, FromRgb)
{
  EXPECT_EQ(Hsv(  0.0, 0.00, 0.00), Hsv(Rgb(  0,   0,   0)));
  EXPECT_EQ(Hsv(  0.0, 1.00, 0.01), Hsv(Rgb(  3,   0,   0)));
  EXPECT_EQ(Hsv(  0.0, 1.00, 0.99), Hsv(Rgb(252,   0,   0)));
  EXPECT_EQ(Hsv(  0.0, 1.00, 1.00), Hsv(Rgb(255,   0,   0)));
  EXPECT_EQ(Hsv( 60.0, 1.00, 0.75), Hsv(Rgb(191, 191,   0)));
  EXPECT_EQ(Hsv(120.0, 1.00, 0.50), Hsv(Rgb(  0, 128,   0)));
  EXPECT_EQ(Hsv(120.0, 1.00, 1.00), Hsv(Rgb(  0, 255,   0)));
  EXPECT_EQ(Hsv(180.0, 0.50, 1.00), Hsv(Rgb(128, 255, 255)));
  EXPECT_EQ(Hsv(240.0, 0.50, 1.00), Hsv(Rgb(128, 128, 255)));
  EXPECT_EQ(Hsv(240.0, 1.00, 1.00), Hsv(Rgb(  0,   0, 255)));
  EXPECT_EQ(Hsv(300.0, 0.66, 0.75), Hsv(Rgb(191,  64, 191)));
  EXPECT_EQ(Hsv(360.0, 1.00, 0.99), Hsv(Rgb(252,   0,   0)));
  EXPECT_EQ(Hsv(360.0, 1.00, 1.00), Hsv(Rgb(255,   0,   0)));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
