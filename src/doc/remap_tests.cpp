// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/remap.h"

using namespace doc;

TEST(Remap, Basics)
{
  std::vector<bool> entries(20);
  std::fill(entries.begin(), entries.end(), false);
  entries[6] =
    entries[7] =
    entries[14] = true;

  Remap map = Remap::moveSelectedEntriesTo(entries, 1);

  EXPECT_EQ(0, map[0]);
  EXPECT_EQ(4, map[1]);
  EXPECT_EQ(5, map[2]);
  EXPECT_EQ(6, map[3]);
  EXPECT_EQ(7, map[4]);
  EXPECT_EQ(8, map[5]);
  EXPECT_EQ(1, map[6]);
  EXPECT_EQ(2, map[7]);
  EXPECT_EQ(9, map[8]);
  EXPECT_EQ(10, map[9]);
  EXPECT_EQ(11, map[10]);
  EXPECT_EQ(12, map[11]);
  EXPECT_EQ(13, map[12]);
  EXPECT_EQ(14, map[13]);
  EXPECT_EQ(3, map[14]);
  EXPECT_EQ(15, map[15]);
  EXPECT_EQ(16, map[16]);
  EXPECT_EQ(17, map[17]);
  EXPECT_EQ(18, map[18]);
  EXPECT_EQ(19, map[19]);

  map = Remap::moveSelectedEntriesTo(entries, 18);

  EXPECT_EQ(0, map[0]);
  EXPECT_EQ(1, map[1]);
  EXPECT_EQ(2, map[2]);
  EXPECT_EQ(3, map[3]);
  EXPECT_EQ(4, map[4]);
  EXPECT_EQ(5, map[5]);
  EXPECT_EQ(15, map[6]);
  EXPECT_EQ(16, map[7]);
  EXPECT_EQ(6, map[8]);
  EXPECT_EQ(7, map[9]);
  EXPECT_EQ(8, map[10]);
  EXPECT_EQ(9, map[11]);
  EXPECT_EQ(10, map[12]);
  EXPECT_EQ(11, map[13]);
  EXPECT_EQ(17, map[14]);
  EXPECT_EQ(12, map[15]);
  EXPECT_EQ(13, map[16]);
  EXPECT_EQ(14, map[17]);
  EXPECT_EQ(18, map[18]);
  EXPECT_EQ(19, map[19]);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
