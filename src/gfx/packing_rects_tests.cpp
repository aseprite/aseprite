// Aseprite Gfx Library
// Copyright (C) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "gfx/packing_rects.h"
#include "gfx/rect_io.h"
#include "gfx/size.h"

using namespace gfx;

TEST(PackingRects, Simple)
{
  PackingRects pr;
  pr.add(Size(256, 128));
  EXPECT_FALSE(pr.pack(Size(256, 120)));
  EXPECT_TRUE(pr.pack(Size(256, 128)));

  EXPECT_EQ(Rect(0, 0, 256, 128), pr[0]);
  EXPECT_EQ(Rect(0, 0, 256, 128), pr.bounds());
}

TEST(PackingRects, SimpleTwoRects)
{
  PackingRects pr;
  pr.add(Size(256, 128));
  pr.add(Size(256, 120));
  EXPECT_TRUE(pr.pack(Size(256, 256)));

  EXPECT_EQ(Rect(0, 0, 256, 256), pr.bounds());
  EXPECT_EQ(Rect(0, 0, 256, 128), pr[0]);
  EXPECT_EQ(Rect(0, 128, 256, 120), pr[1]);
}

TEST(PackingRects, BestFit)
{
  PackingRects pr;
  pr.add(Size(10, 12));
  pr.bestFit();
  EXPECT_EQ(Rect(0, 0, 16, 16), pr.bounds());
}

TEST(PackingRects, BestFitTwoRects)
{
  PackingRects pr;
  pr.add(Size(256, 128));
  pr.add(Size(256, 127));
  pr.bestFit();

  EXPECT_EQ(Rect(0, 0, 256, 256), pr.bounds());
  EXPECT_EQ(Rect(0, 0, 256, 128), pr[0]);
  EXPECT_EQ(Rect(0, 128, 256, 127), pr[1]);
}

TEST(PackingRects, BestFit6Frames100x100)
{
  PackingRects pr;
  pr.add(Size(100, 100));
  pr.add(Size(100, 100));
  pr.add(Size(100, 100));
  pr.add(Size(100, 100));
  pr.add(Size(100, 100));
  pr.add(Size(100, 100));
  pr.bestFit();

  EXPECT_EQ(Rect(0, 0, 512, 256), pr.bounds());
  EXPECT_EQ(Rect(0, 0, 100, 100), pr[0]);
  EXPECT_EQ(Rect(100, 0, 100, 100), pr[1]);
  EXPECT_EQ(Rect(200, 0, 100, 100), pr[2]);
  EXPECT_EQ(Rect(300, 0, 100, 100), pr[3]);
  EXPECT_EQ(Rect(400, 0, 100, 100), pr[4]);
  EXPECT_EQ(Rect(0, 100, 100, 100), pr[5]);
}

TEST(PackingRects, KeepSameRectsOrder)
{
  PackingRects pr;
  pr.add(Size(10, 10));
  pr.add(Size(20, 20));
  pr.add(Size(30, 30));
  pr.bestFit();

  EXPECT_EQ(Rect(0, 0, 64, 32), pr.bounds());
  EXPECT_EQ(Rect(50, 0, 10, 10), pr[0]);
  EXPECT_EQ(Rect(30, 0, 20, 20), pr[1]);
  EXPECT_EQ(Rect(0, 0, 30, 30), pr[2]);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
