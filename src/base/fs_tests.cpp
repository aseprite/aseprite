// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/fs.h"

using namespace base;

TEST(FileSystem, MakeDirectory)
{
  EXPECT_FALSE(is_directory("a"));

  make_directory("a");
  EXPECT_TRUE(is_directory("a"));

  remove_directory("a");
  EXPECT_FALSE(is_directory("a"));
}

TEST(FileSystem, MakeAllDirectories)
{
  EXPECT_FALSE(is_directory("a"));
  EXPECT_FALSE(is_directory("a/b"));
  EXPECT_FALSE(is_directory("a/b/c"));

  make_all_directories("a/b/c");

  EXPECT_TRUE(is_directory("a"));
  EXPECT_TRUE(is_directory("a/b"));
  EXPECT_TRUE(is_directory("a/b/c"));

  remove_directory("a/b/c");
  EXPECT_FALSE(is_directory("a/b/c"));

  remove_directory("a/b");
  EXPECT_FALSE(is_directory("a/b"));

  remove_directory("a");
  EXPECT_FALSE(is_directory("a"));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
