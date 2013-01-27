// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "base/split_string.h"

TEST(SplitString, Empty)
{
  std::vector<base::string> result;
  base::split_string("", result, ",");
  ASSERT_EQ(1, result.size());
  EXPECT_EQ("", result[0]);
}

TEST(SplitString, NoSeparator)
{
  std::vector<base::string> result;
  base::split_string("Hello,World", result, "");
  ASSERT_EQ(1, result.size());
  EXPECT_EQ("Hello,World", result[0]);
}

TEST(SplitString, OneSeparator)
{
  std::vector<base::string> result;
  base::split_string("Hello,World", result, ",");
  ASSERT_EQ(2, result.size());
  EXPECT_EQ("Hello", result[0]);
  EXPECT_EQ("World", result[1]);
}

TEST(SplitString, MultipleSeparators)
{
  std::vector<base::string> result;
  base::split_string("Hello,World", result, ",r");
  ASSERT_EQ(3, result.size());
  EXPECT_EQ("Hello", result[0]);
  EXPECT_EQ("Wo", result[1]);
  EXPECT_EQ("ld", result[2]);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
