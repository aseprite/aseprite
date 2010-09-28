// ASE base library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "tests/test.h"

#include <string>
#include <vector>

#include "base/split_string.h"

TEST(SplitString, Empty)
{
  std::vector<std::string> result;
  base::split_string<std::string>("", result, ",");
  ASSERT_EQ(1, result.size());
  EXPECT_EQ("", result[0]);
}

TEST(SplitString, NoSeparator)
{
  std::vector<std::string> result;
  base::split_string<std::string>("Hello,World", result, "");
  ASSERT_EQ(1, result.size());
  EXPECT_EQ("Hello,World", result[0]);
}

TEST(SplitString, OneSeparator)
{
  std::vector<std::string> result;
  base::split_string<std::string>("Hello,World", result, ",");
  ASSERT_EQ(2, result.size());
  EXPECT_EQ("Hello", result[0]);
  EXPECT_EQ("World", result[1]);
}

TEST(SplitString, MultipleSeparators)
{
  std::vector<std::string> result;
  base::split_string<std::string>("Hello,World", result, ",r");
  ASSERT_EQ(3, result.size());
  EXPECT_EQ("Hello", result[0]);
  EXPECT_EQ("Wo", result[1]);
  EXPECT_EQ("ld", result[2]);
}
