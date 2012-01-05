// ASE gfx library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "gfx/rect.h"
#include "gfx/size.h"
#include "gfx/border.h"

using namespace std;
using namespace gfx;

ostream& operator<<(ostream& os, const Rect& rect)
{
  return os << "("
            << rect.x << ", "
            << rect.y << ", "
            << rect.w << ", "
            << rect.h << ")";
}

TEST(Rect, Ctor)
{
  EXPECT_EQ(Rect(0, 0, 0, 0), Rect());
  EXPECT_EQ(10, Rect(10, 20, 30, 40).x);
  EXPECT_EQ(20, Rect(10, 20, 30, 40).y);
  EXPECT_EQ(30, Rect(10, 20, 30, 40).w);
  EXPECT_EQ(40, Rect(10, 20, 30, 40).h);
}

TEST(Rect, Inflate)
{
  EXPECT_EQ(Rect(10, 20, 31, 42), Rect(10, 20, 30, 40).inflate(1, 2));
  EXPECT_EQ(Rect(10, 20, 31, 42), Rect(10, 20, 30, 40).inflate(Size(1, 2)));
}

TEST(Rect, Enlarge)
{
  EXPECT_EQ(Rect(9, 19, 32, 42), Rect(10, 20, 30, 40).enlarge(1));
  EXPECT_EQ(Rect(9, 18, 34, 46), Rect(10, 20, 30, 40).enlarge(Border(1, 2, 3, 4)));
  EXPECT_EQ(Rect(9, 18, 34, 46), Rect(10, 20, 30, 40) + Border(1, 2, 3, 4));
}

TEST(Rect, Shrink)
{
  EXPECT_EQ(Rect(11, 21, 28, 38), Rect(10, 20, 30, 40).shrink(1));
  EXPECT_EQ(Rect(11, 22, 26, 34), Rect(10, 20, 30, 40).shrink(Border(1, 2, 3, 4)));
  EXPECT_EQ(Rect(11, 22, 26, 34), Rect(10, 20, 30, 40) - Border(1, 2, 3, 4));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
