// Aseprite Document Library
// Copyright (c) 2016-2018 David Capello
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

static std::vector<frame_t> to_vector(const SelectedFrames& f)
{
  std::vector<frame_t> v;
  std::copy(f.begin(), f.end(), std::back_inserter(v));
  return v;
}

TEST(SelectedFrames, BasicOneRange)
{
  SelectedFrames f;
  EXPECT_TRUE(f.empty());
  f.insert(1);
  f.insert(2);
  f.insert(3);
  EXPECT_FALSE(f.empty());
  EXPECT_EQ(3, f.size());
  EXPECT_EQ(1, f.ranges());

  auto res = to_vector(f);
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
  EXPECT_EQ(3, f.size());
  EXPECT_EQ(3, f.ranges());

  auto res = to_vector(f);
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
  EXPECT_EQ(5, f.size());
  EXPECT_EQ(2, f.ranges());
  EXPECT_EQ(3, f.firstFrame());
  EXPECT_EQ(8, f.lastFrame());

  auto res = to_vector(f);
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
  EXPECT_EQ(6, f.size());
  EXPECT_EQ(3, f.ranges());

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

TEST(SelectedFrames, ReverseIterators)
{
  SelectedFrames f;
  f.insert(1);
  f.insert(5, 7);
  EXPECT_EQ(4, f.size());
  EXPECT_EQ(2, f.ranges());

  std::vector<frame_t> res;
  std::copy(f.rbegin(), f.rend(), std::back_inserter(res));

  ASSERT_EQ(4, res.size());
  EXPECT_EQ(7, res[0]);
  EXPECT_EQ(6, res[1]);
  EXPECT_EQ(5, res[2]);
  EXPECT_EQ(1, res[3]);

  std::vector<frame_t> res2;
  for (frame_t frame : f.reversed())
    res2.push_back(frame);

  EXPECT_EQ(res, res2);
}

TEST(SelectedFrames, MakeReverseSimple)
{
  SelectedFrames f;
  f.insert(4, 9);
  EXPECT_EQ(6, f.size());
  EXPECT_EQ(1, f.ranges());

  f = f.makeReverse();
  EXPECT_EQ(6, f.size());
  EXPECT_EQ(1, f.ranges());

  auto res = to_vector(f);
  ASSERT_EQ(6, res.size());
  EXPECT_EQ(9, res[0]);
  EXPECT_EQ(8, res[1]);
  EXPECT_EQ(7, res[2]);
  EXPECT_EQ(6, res[3]);
  EXPECT_EQ(5, res[4]);
  EXPECT_EQ(4, res[5]);
}

TEST(SelectedFrames, MakeReverse)
{
  SelectedFrames f;
  f.insert(1);
  f.insert(4, 5);
  f.insert(7, 9);
  EXPECT_EQ(6, f.size());
  EXPECT_EQ(3, f.ranges());

  f = f.makeReverse();
  EXPECT_EQ(3, f.ranges());

  auto res = to_vector(f);
  ASSERT_EQ(6, res.size());
  EXPECT_EQ(9, res[0]);
  EXPECT_EQ(8, res[1]);
  EXPECT_EQ(7, res[2]);
  EXPECT_EQ(5, res[3]);
  EXPECT_EQ(4, res[4]);
  EXPECT_EQ(1, res[5]);
}

TEST(SelectedFrames, MakePingPongAndFilter)
{
  SelectedFrames f;
  f.insert(1);
  f.insert(4, 5);
  f.insert(7, 9);
  EXPECT_EQ(6, f.size());
  EXPECT_EQ(3, f.ranges());

  f = f.makePingPong();
  EXPECT_EQ(5, f.ranges());

  auto res = to_vector(f);
  ASSERT_EQ(10, res.size());
  EXPECT_EQ(1, res[0]);
  EXPECT_EQ(4, res[1]);
  EXPECT_EQ(5, res[2]);
  EXPECT_EQ(7, res[3]);
  EXPECT_EQ(8, res[4]);
  EXPECT_EQ(9, res[5]);
  EXPECT_EQ(8, res[6]);
  EXPECT_EQ(7, res[7]);
  EXPECT_EQ(5, res[8]);
  EXPECT_EQ(4, res[9]);

  f = f.filter(5, 8);
  EXPECT_EQ(4, f.ranges());
  res = to_vector(f);
  EXPECT_EQ(6, res.size());
  EXPECT_EQ(5, res[0]);
  EXPECT_EQ(7, res[1]);
  EXPECT_EQ(8, res[2]);
  EXPECT_EQ(8, res[3]);
  EXPECT_EQ(7, res[4]);
  EXPECT_EQ(5, res[5]);

  f = f.filter(7, 7);
  EXPECT_EQ(2, f.ranges());
  res = to_vector(f);
  EXPECT_EQ(2, res.size());
  EXPECT_EQ(7, res[0]);
  EXPECT_EQ(7, res[1]);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
