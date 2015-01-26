// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/replace_string.h"

inline std::string rs(const std::string& s, const std::string& a, const std::string& b) {
  std::string res = s;
  base::replace_string(res, a, b);
  return res;
}

TEST(ReplaceString, Basic)
{
  EXPECT_EQ("", rs("", "", ""));
  EXPECT_EQ("aa", rs("ab", "b", "a"));
  EXPECT_EQ("abc", rs("accc", "cc", "b"));
  EXPECT_EQ("abb", rs("acccc", "cc", "b"));
  EXPECT_EQ("aabbbbaabbbb", rs("aabbaabb", "bb", "bbbb"));
  EXPECT_EQ("123123123", rs("111", "1", "123"));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
