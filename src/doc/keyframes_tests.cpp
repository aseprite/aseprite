// Aseprite Document Library
// Copyright (c) 2022 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/keyframes.h"

using namespace doc;

TEST(Keyframes, Operations)
{
  Keyframes<int> k;

  EXPECT_TRUE(k.empty());
  EXPECT_EQ(0, k.size());

  k.insert(0, std::make_unique<int>(5));
  EXPECT_FALSE(k.empty());
  EXPECT_EQ(1, k.size());
  EXPECT_EQ(0, k.fromFrame());
  EXPECT_EQ(0, k.toFrame());
  EXPECT_EQ(nullptr, k[-1]);
  EXPECT_EQ(5, *k[0]);
  EXPECT_EQ(5, *k[1]);
  EXPECT_EQ(5, *k[2]);

  k.insert(2, std::make_unique<int>(6));
  EXPECT_EQ(2, k.size());
  EXPECT_EQ(0, k.fromFrame());
  EXPECT_EQ(2, k.toFrame());
  EXPECT_EQ(nullptr, k[-1]);
  EXPECT_EQ(5, *k[0]);
  EXPECT_EQ(5, *k[1]);
  EXPECT_EQ(6, *k[2]);
  EXPECT_EQ(6, *k[3]);

  k.insert(1, std::make_unique<int>(3));
  EXPECT_EQ(3, k.size());
  EXPECT_EQ(0, k.fromFrame());
  EXPECT_EQ(2, k.toFrame());
  EXPECT_EQ(5, *k[0]);
  EXPECT_EQ(3, *k[1]);
  EXPECT_EQ(6, *k[2]);
  EXPECT_EQ(6, *k[3]);

  k.remove(0);
  EXPECT_EQ(2, k.size());
  EXPECT_EQ(1, k.fromFrame());
  EXPECT_EQ(2, k.toFrame());
  EXPECT_EQ(nullptr, k[0]);
  EXPECT_EQ(3, *k[1]);
  EXPECT_EQ(6, *k[2]);
  EXPECT_EQ(6, *k[3]);

  k.remove(3);
  EXPECT_EQ(1, k.size());
  EXPECT_EQ(1, k.fromFrame());
  EXPECT_EQ(1, k.toFrame());
  EXPECT_EQ(nullptr, k[0]);
  EXPECT_EQ(3, *k[1]);
  EXPECT_EQ(3, *k[2]);
  EXPECT_EQ(3, *k[3]);
}

TEST(Keyframes, Range)
{
  Keyframes<int> k;
  k.insert(0, std::make_unique<int>(5));
  k.insert(2, nullptr);
  k.insert(4, std::make_unique<int>(8));
  k.insert(6, std::make_unique<int>(2));

  EXPECT_EQ(0, k.fromFrame());
  EXPECT_EQ(6, k.toFrame());

  EXPECT_EQ(5, *k[0]);
  EXPECT_EQ(5, *k[1]);
  EXPECT_EQ(nullptr, k[2]);
  EXPECT_EQ(nullptr, k[3]);
  EXPECT_EQ(8, *k[4]);
  EXPECT_EQ(8, *k[5]);
  EXPECT_EQ(2, *k[6]);
  EXPECT_EQ(2, *k[7]);

  int pass = 0;
  for (int* value : k.range(0, 0)) {
    EXPECT_EQ(5, *value);
    ++pass;
  }
  EXPECT_EQ(1, pass);

  frame_t f = 1;
  for (int* value : k.range(1, 5)) {
    switch (f) {
      case 1:  EXPECT_EQ(5, *value); break;
      case 2:  EXPECT_EQ(nullptr, value); break;
      case 3:  EXPECT_EQ(nullptr, value); break;
      case 4:  EXPECT_EQ(8, *value); break;
      case 5:  EXPECT_EQ(8, *value); break;
      default: ASSERT_TRUE(false);
    }
    ++f;
  }
  EXPECT_EQ(f, 6);

  EXPECT_TRUE(k.range(-3, -1).empty());
  EXPECT_FALSE(k.range(0, 1).empty());
  EXPECT_FALSE(k.range(2, 3).empty());
  EXPECT_FALSE(k.range(7, 7).empty());

  EXPECT_EQ(1, k.range(0, 0).countKeys());
  EXPECT_EQ(1, k.range(0, 1).countKeys());
  EXPECT_EQ(2, k.range(0, 2).countKeys());
  EXPECT_EQ(4, k.range(0, 7).countKeys());
  EXPECT_EQ(1, k.range(2, 3).countKeys());
  EXPECT_EQ(2, k.range(2, 4).countKeys());
  EXPECT_EQ(3, k.range(3, 7).countKeys());
  EXPECT_EQ(2, k.range(5, 6).countKeys());
  EXPECT_EQ(1, k.range(6, 6).countKeys());
  EXPECT_EQ(1, k.range(7, 7).countKeys());
}

TEST(Keyframes, BugEmptyCount)
{
  Keyframes<int> k;
  k.insert(7, std::make_unique<int>(5));
  EXPECT_EQ(0, k.range(-1, 6).countKeys());
  EXPECT_EQ(1, k.range(0, 7).countKeys());
  EXPECT_EQ(1, k.range(8, 9).countKeys());
  EXPECT_EQ(5, **k.range(8, 9).begin());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
