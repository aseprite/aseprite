// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/frames_sequence.h"

#include <algorithm>
#include <iterator>

using namespace doc;

static std::vector<frame_t> to_vector(const FramesSequence& f)
{
  std::vector<frame_t> v;
  std::copy(f.begin(), f.end(), std::back_inserter(v));
  return v;
}

TEST(FramesSequence, BasicOneRange)
{
  FramesSequence f;
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

TEST(FramesSequence, BasicThreeRanges)
{
  FramesSequence f;
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

TEST(FramesSequence, InsertDecreasingFramesSequenceAfterIncreasingSequence)
{
  FramesSequence f;
  f.insert(3);
  f.insert(5, 8);
  f.insert(7, 4);
  f.insert(3);
  f.insert(1);
  f.insert(2);
  EXPECT_EQ(12, f.size());
  EXPECT_EQ(4, f.ranges());
  EXPECT_EQ(3, f.firstFrame());
  EXPECT_EQ(2, f.lastFrame());
  EXPECT_EQ(1, f.lowestFrame());

  auto res = to_vector(f);
  ASSERT_EQ(12, res.size());
  EXPECT_EQ(3, res[0]);
  EXPECT_EQ(5, res[1]);
  EXPECT_EQ(6, res[2]);
  EXPECT_EQ(7, res[3]);
  EXPECT_EQ(8, res[4]);
  EXPECT_EQ(7, res[5]);
  EXPECT_EQ(6, res[6]);
  EXPECT_EQ(5, res[7]);
  EXPECT_EQ(4, res[8]);
  EXPECT_EQ(3, res[9]);
  EXPECT_EQ(1, res[10]);
  EXPECT_EQ(2, res[11]);
}

TEST(FramesSequence, Contains)
{
  FramesSequence f;
  f.insert(1);
  f.insert(4, 5);
  f.insert(9, 7);
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

TEST(FramesSequence, ReverseIterators)
{
  FramesSequence f;
  f.insert(1);
  f.insert(5, 7);
  f.insert(8, 2);
  EXPECT_EQ(11, f.size());
  EXPECT_EQ(3, f.ranges());

  std::vector<frame_t> res;
  std::copy(f.rbegin(), f.rend(), std::back_inserter(res));

  ASSERT_EQ(11, res.size());
  EXPECT_EQ(2, res[0]);
  EXPECT_EQ(3, res[1]);
  EXPECT_EQ(4, res[2]);
  EXPECT_EQ(5, res[3]);
  EXPECT_EQ(6, res[4]);
  EXPECT_EQ(7, res[5]);
  EXPECT_EQ(8, res[6]);
  EXPECT_EQ(7, res[7]);
  EXPECT_EQ(6, res[8]);
  EXPECT_EQ(5, res[9]);
  EXPECT_EQ(1, res[10]);

  std::vector<frame_t> res2;
  for (frame_t frame : f.reversed())
    res2.push_back(frame);

  EXPECT_EQ(res, res2);
}

TEST(FramesSequence, MakeReverseSimple)
{
  FramesSequence f;
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

TEST(FramesSequence, MakeReverse)
{
  FramesSequence f;
  f.insert(1);
  f.insert(4, 5);
  f.insert(5, 1);
  f.insert(7);
  f.insert(9, 5);
  EXPECT_EQ(14, f.size());
  EXPECT_EQ(5, f.ranges());

  f = f.makeReverse();
  EXPECT_EQ(5, f.ranges());

  auto res = to_vector(f);
  ASSERT_EQ(14, res.size());
  EXPECT_EQ(5, res[0]);
  EXPECT_EQ(6, res[1]);
  EXPECT_EQ(7, res[2]);
  EXPECT_EQ(8, res[3]);
  EXPECT_EQ(9, res[4]);
  EXPECT_EQ(7, res[5]);
  EXPECT_EQ(1, res[6]);
  EXPECT_EQ(2, res[7]);
  EXPECT_EQ(3, res[8]);
  EXPECT_EQ(4, res[9]);
  EXPECT_EQ(5, res[10]);
  EXPECT_EQ(5, res[11]);
  EXPECT_EQ(4, res[12]);
  EXPECT_EQ(1, res[13]);
}

TEST(FramesSequence, MakePingPongAndFilter)
{
  FramesSequence f;
  f.insert(1);
  f.insert(4, 5);
  f.insert(9, 7);
  f.insert(8);
  EXPECT_EQ(7, f.size());
  EXPECT_EQ(4, f.ranges());

  f = f.makePingPong();
  EXPECT_EQ(6, f.ranges());

  auto res = to_vector(f);
  ASSERT_EQ(12, res.size());
  EXPECT_EQ(1, res[0]);
  EXPECT_EQ(4, res[1]);
  EXPECT_EQ(5, res[2]);
  EXPECT_EQ(9, res[3]);
  EXPECT_EQ(8, res[4]);
  EXPECT_EQ(7, res[5]);
  EXPECT_EQ(8, res[6]);
  EXPECT_EQ(7, res[7]);
  EXPECT_EQ(8, res[8]);
  EXPECT_EQ(9, res[9]);
  EXPECT_EQ(5, res[10]);
  EXPECT_EQ(4, res[11]);

  f = f.filter(5, 8);
  EXPECT_EQ(5, f.ranges());
  res = to_vector(f);
  EXPECT_EQ(7, res.size());
  EXPECT_EQ(5, res[0]);
  EXPECT_EQ(8, res[1]);
  EXPECT_EQ(7, res[2]);
  EXPECT_EQ(8, res[3]);
  EXPECT_EQ(7, res[4]);
  EXPECT_EQ(8, res[5]);
  EXPECT_EQ(5, res[6]);

  f = f.filter(7, 7);
  EXPECT_EQ(2, f.ranges());
  res = to_vector(f);
  EXPECT_EQ(2, res.size());
  EXPECT_EQ(7, res[0]);
  EXPECT_EQ(7, res[1]);
}

TEST(FramesSequence, RangeMerging)
{
  FramesSequence f;
  f.insert(1);
  f.insert(2, 3);
  f.insert(4);
  f.insert(5, 7);
  f.insert(8);
  f.insert(9, 5);
  f.insert(4);
  f.insert(3, 2);
  f.insert(1);
  EXPECT_EQ(17, f.size());
  EXPECT_EQ(2, f.ranges());

  f.clear();

  f.insert(10, 8);
  f.insert(7);
  f.insert(6, 4);
  f.insert(3);
  f.insert(2, 1);
  f.insert(2);
  f.insert(3, 5);
  f.insert(6);
  f.insert(7, 8);
  f.insert(9, 10);
  EXPECT_EQ(19, f.size());
  EXPECT_EQ(2, f.ranges());
}

TEST(FramesSequence, SelectedFramesConversion)
{
  SelectedFrames sf;
  sf.insert(3);
  sf.insert(5, 8);
  sf.insert(7);
  sf.insert(9);
  EXPECT_EQ(6, sf.size());
  EXPECT_EQ(2, sf.ranges());
  EXPECT_EQ(3, sf.firstFrame());
  EXPECT_EQ(9, sf.lastFrame());

  FramesSequence f(sf);
  EXPECT_EQ(sf.size(), f.size());
  EXPECT_EQ(sf.ranges(), f.ranges());
  EXPECT_EQ(sf.firstFrame(), f.firstFrame());
  EXPECT_EQ(sf.lastFrame(), f.lastFrame());

  auto ressf = to_vector(sf);
  auto resf = to_vector(f);
  EXPECT_EQ(ressf.size(), resf.size());
  for (int i=0; i < resf.size(); ++i) {
    EXPECT_EQ(ressf[i], resf[i]);
  }
}

TEST(FramesSequence, Displace)
{
  FramesSequence f;
  f.insert(1);
  f.insert(4, 5);
  f.insert(9, 7);
  f.insert(8);
  EXPECT_EQ(7, f.size());
  EXPECT_EQ(4, f.ranges());

  f.displace(4);
  auto res = to_vector(f);
  ASSERT_EQ(7, res.size());
  EXPECT_EQ(5, res[0]);
  EXPECT_EQ(8, res[1]);
  EXPECT_EQ(9, res[2]);
  EXPECT_EQ(13, res[3]);
  EXPECT_EQ(12, res[4]);
  EXPECT_EQ(11, res[5]);
  EXPECT_EQ(12, res[6]);

  f.clear();

  f.insert(3);
  f.insert(4, 5);
  f.insert(9, 7);
  f.insert(2);
  EXPECT_EQ(7, f.size());
  EXPECT_EQ(3, f.ranges());

  f.displace(-4);
  // Check that it was displaced just -2 frames because it is the lowest frame
  // in the sequence.
  res = to_vector(f);
  ASSERT_EQ(7, res.size());
  EXPECT_EQ(1, res[0]);
  EXPECT_EQ(2, res[1]);
  EXPECT_EQ(3, res[2]);
  EXPECT_EQ(7, res[3]);
  EXPECT_EQ(6, res[4]);
  EXPECT_EQ(5, res[5]);
  EXPECT_EQ(0, res[6]);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
