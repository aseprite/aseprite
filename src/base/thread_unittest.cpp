// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/thread.h"

using namespace base;

void nothing() { }

TEST(Thread, NotJoinable)
{
  thread t;
  EXPECT_FALSE(t.joinable());
}

TEST(Thread, Joinable)
{
  thread t(&nothing);
  EXPECT_TRUE(t.joinable());
  t.join();
}

//////////////////////////////////////////////////////////////////////

bool flag = false;

void func0() {
  flag = true;
}

void func1(int x) {
  flag = true;
  EXPECT_EQ(2, x);
}

void func2(int x, int y) {
  flag = true;
  EXPECT_EQ(2, x);
  EXPECT_EQ(4, y);
}

TEST(Thread, WithoutArgs)
{
  flag = false;
  thread t(&func0);
  t.join();
  EXPECT_TRUE(flag);
}

TEST(Thread, OneArg)
{
  flag = false;
  thread t(&func1, 2);
  t.join();
  EXPECT_TRUE(flag);
}

TEST(Thread, TwoArgs)
{
  flag = false;
  thread t(&func2, 2, 4);
  t.join();
  EXPECT_TRUE(flag);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
