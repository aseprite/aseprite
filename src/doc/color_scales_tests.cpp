// Aseprite Document Library
// Copyright (c) 2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/color_scales.h"

#include <cmath>
#include <cstdlib>

using namespace doc;

TEST(ColorScales, MatchValues)
{
  for (int v = 0; v < 8; ++v)
    EXPECT_EQ(scale_3bits_to_8bits(v), scale_xbits_to_8bits(3, v));

  for (int v = 0; v < 32; ++v)
    EXPECT_EQ(scale_5bits_to_8bits(v), scale_xbits_to_8bits(5, v));

  for (int v = 0; v < 64; ++v)
    EXPECT_EQ(scale_6bits_to_8bits(v), scale_xbits_to_8bits(6, v));

  for (int x = 1; x <= 8; ++x) {
    for (int v = 0; v < (1 << x); ++v)
      EXPECT_LE(std::abs((255 * v / ((1 << x) - 1)) - scale_xbits_to_8bits(x, v)), 1);
  }
}

TEST(ColorScales, TenBits)
{
  EXPECT_EQ(0xff, scale_xbits_to_8bits(10, 0x3ff));
  EXPECT_EQ(0x7f, scale_xbits_to_8bits(10, 0x1ff));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
