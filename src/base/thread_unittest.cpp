// ASE base library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "tests/test.h"

#include "base/thread.h"

#include <iostream>

using std::cout;
using namespace base;

static bool flag = false;

static void func0() {
  flag = true;
}

static void func1(int x) {
  flag = true;
  EXPECT_EQ(2, x);
}

static void func2(int x, int y) {
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

