// Aseprite Document Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/selected_frames.h"

#include <algorithm>
#include <iterator>

using namespace doc;

TEST(SelectedFrames, BasicOneRange)
{
  SelectedFrames f;
  ASSERT_TRUE(f.empty());
  f.insert(1);
  f.insert(2);
  f.insert(3);
  ASSERT_FALSE(f.empty());
  ASSERT_EQ(3, f.size());

  std::vector<frame_t> res;
  std::copy(f.begin(), f.end(), std::back_inserter(res));

  ASSERT_EQ(3, res.size());
  EXPECT_EQ(1, res[0]);
  EXPECT_EQ(2, res[1]);
  EXPECT_EQ(3, res[2]);
}

TEST(SelectedFrames, BasicThreeRanges)
{
  SelectedFrames f;
  f.insert(1);
  f.insert(3);
  f.insert(5);
  ASSERT_EQ(3, f.size());

  std::vector<frame_t> res;
  std::copy(f.begin(), f.end(), std::back_inserter(res));

  ASSERT_EQ(3, res.size());
  EXPECT_EQ(1, res[0]);
  EXPECT_EQ(3, res[1]);
  EXPECT_EQ(5, res[2]);
}

TEST(SelectedFrames, InsertSelectedFrameInsideSelectedRange)
{
  SelectedFrames f;
  f.insert(3);
  f.insert(5, 8);
  f.insert(7);
  ASSERT_EQ(5, f.size());

  std::vector<frame_t> res;
  std::copy(f.begin(), f.end(), std::back_inserter(res));

  ASSERT_EQ(5, res.size());
  EXPECT_EQ(3, res[0]);
  EXPECT_EQ(5, res[1]);
  EXPECT_EQ(6, res[2]);
  EXPECT_EQ(7, res[3]);
  EXPECT_EQ(8, res[4]);
}

TEST(SelectedFrames, Contains)
{
  SelectedFrames f;
  f.insert(1);
  f.insert(4, 5);
  f.insert(7, 9);

  EXPECT_FALSE(f.contains(0));
  EXPECT_TRUE(f.contains(1));
  EXPECT_FALSE(f.contains(2));
  EXPECT_FALSE(f.contains(3));
  EXPECT_TRUE(f.contains(4));
  EXPECT_TRUE(f.contains(5));
  EXPECT_FALSE(f.contains(6));
  EXPECT_TRUE(f.contains(7));
  EXPECT_TRUE(f.contains(8));
  EXPECT_TRUE(f.contains(9));
  EXPECT_FALSE(f.contains(10));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
