// Aseprite Render Library
// Copyright (c) 2019 Igara Studio S.A.
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "render/dithering_matrix.h"
#include "render/ordered_dither.h"

using namespace doc;
using namespace render;

TEST(BayerMatrix, CheckD2)
{
  BayerMatrix matrix(2);
  int expected[2*2] = {
    0, 2,
    3, 1
  };
  int c=0;
  for (int i=0; i<2; ++i)
    for (int j=0; j<2; ++j)
      EXPECT_EQ(expected[c++], matrix(i, j));
}

TEST(BayerMatrix, CheckD4)
{
  BayerMatrix matrix(4);
  int expected[4*4] = {
     0,   8,  2, 10,
    12,   4, 14,  6,
     3,  11,  1,  9,
    15,   7, 13,  5
  };
  int c=0;
  for (int i=0; i<4; ++i)
    for (int j=0; j<4; ++j)
      EXPECT_EQ(expected[c++], matrix(i, j));
}

TEST(BayerMatrix, CheckD8)
{
  BayerMatrix matrix(8);
  int expected[8*8] = {
     0, 32,  8, 40,    2, 34, 10, 42,
    48, 16, 56, 24,   50, 18, 58, 26,
    12, 44,  4, 36,   14, 46,  6, 38,
    60, 28, 52, 20,   62, 30, 54, 22,

     3, 35, 11, 43,    1, 33,  9, 41,
    51, 19, 59, 27,   49, 17, 57, 25,
    15, 47,  7, 39,   13, 45,  5, 37,
    63, 31, 55, 23,   61, 29, 53, 21
  };
  int c=0;
  for (int i=0; i<8; ++i)
    for (int j=0; j<8; ++j)
      EXPECT_EQ(expected[c++], matrix(i, j));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
