// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/unique_ptr.h"

using namespace base;

TEST(UniquePtr, DefaultCtor)
{
  UniquePtr<int> a;
  EXPECT_TRUE(NULL == a.get());

  a.reset(new int(2));
  ASSERT_TRUE(NULL != a.get());
  EXPECT_TRUE(2 == *a);

  a.reset();
  EXPECT_TRUE(NULL == a.get());
}

TEST(UniquePtr, IntPtr)
{
  UniquePtr<int> a(new int(5));
  EXPECT_EQ(5, *a);
}

TEST(UniquePtr, CopyValues)
{
  UniquePtr<int> a(new int(3));
  UniquePtr<int> b(new int(4));

  EXPECT_EQ(3, *a);
  EXPECT_EQ(4, *b);

  *a = *b;

  EXPECT_EQ(4, *a);
  EXPECT_EQ(4, *b);
}

int valueInDtor;

class A {
  int m_value;
public:
  A(int value) : m_value(value) { }
  ~A() { valueInDtor = m_value; }
};

TEST(UniquePtr, PtrToStruct)
{
  valueInDtor = 0;
  {
    UniquePtr<A> a(new A(100));
    {
      UniquePtr<A> b(new A(200));
      {
        UniquePtr<A> c(new A(300));

        EXPECT_EQ(0, valueInDtor);

        c.reset();
        EXPECT_EQ(300, valueInDtor);

        c.reset(new A(400));
      }
      EXPECT_EQ(400, valueInDtor);
    }
    EXPECT_EQ(200, valueInDtor);
  }
  EXPECT_EQ(100, valueInDtor);
}

TEST(UniquePtr, Release)
{
  UniquePtr<int> a(new int(5));
  EXPECT_EQ(5, *a);

  int* ptr = a.release();
  delete ptr;
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
