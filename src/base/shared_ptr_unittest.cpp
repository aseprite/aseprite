// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/shared_ptr.h"

TEST(SharedPtr, IntPtr)
{
  SharedPtr<int> a(new int(5));
  EXPECT_EQ(5, *a);
}

TEST(SharedPtr, UseCount)
{
  SharedPtr<int> a(new int(5));
  EXPECT_EQ(1, a.use_count());
  EXPECT_TRUE(a.unique());
  a.reset();
  EXPECT_EQ(0, a.use_count());

  SharedPtr<int> b(new int(5));
  {
    SharedPtr<int> c(b);
    EXPECT_EQ(2, b.use_count());
    EXPECT_EQ(2, c.use_count());
    a = c;
    EXPECT_EQ(3, a.use_count());
    EXPECT_EQ(3, b.use_count());
    EXPECT_EQ(3, c.use_count());
    a.reset();
    EXPECT_EQ(2, b.use_count());
    EXPECT_EQ(2, c.use_count());
  }
  EXPECT_EQ(1, b.use_count());
}


class DeleteIsCalled
{
public:
  DeleteIsCalled(bool& flag) : m_flag(flag) { }
  ~DeleteIsCalled() { m_flag = true; }
private:
  bool& m_flag;
};

TEST(SharedPtr, DeleteIsCalled)
{
  bool flag = false;
  {
    SharedPtr<DeleteIsCalled> a(new DeleteIsCalled(flag));
  }
  EXPECT_EQ(true, flag);
}


class A { };
class B : public A { };
class C : public A { };

TEST(SharedPtr, Hierarchy)
{
  {
    SharedPtr<A> a(new B);
    SharedPtr<B> b = a;
    SharedPtr<A> c = a;
    SharedPtr<A> d = b;
    EXPECT_EQ(4, a.use_count());
  }

  {
    SharedPtr<B> b(new B);
    EXPECT_TRUE(b.unique());

    SharedPtr<C> c(new C);
    EXPECT_TRUE(c.unique());

    SharedPtr<A> a = b;
    EXPECT_EQ(2, a.use_count());
    EXPECT_EQ(2, b.use_count());
    EXPECT_FALSE(b.unique());

    a = c;
    EXPECT_EQ(2, a.use_count());
    EXPECT_EQ(1, b.use_count());
    EXPECT_EQ(2, c.use_count());

    a = b;
    EXPECT_EQ(2, a.use_count());
    EXPECT_EQ(2, b.use_count());
    EXPECT_EQ(1, c.use_count());

    EXPECT_TRUE(c.unique());
  }
}

TEST(SharedPtr, Compare)
{
  SharedPtr<int> a(new int(0));
  SharedPtr<int> b(a);
  SharedPtr<int> c(new int(0));

  // Compare pointers
  EXPECT_TRUE(a == b);
  EXPECT_TRUE(a != c);
  EXPECT_TRUE(b != c);

  // Compare pointers
  a = c;
  c = b;
  EXPECT_TRUE(a != b);
  EXPECT_TRUE(a != c);
  EXPECT_TRUE(b == c);

  // Compare values
  EXPECT_TRUE(*a == *b);
  EXPECT_TRUE(*a == *c);
  EXPECT_TRUE(*b == *c);

  // Change values
  *a = 2;
  *b = 5;
  EXPECT_EQ(2, *a);
  EXPECT_EQ(5, *b);
  EXPECT_EQ(5, *c);
}

TEST(SharedPtr, ResetBugDoesntSetPtrToNull)
{
  SharedPtr<int> a(new int(5));
  {
    SharedPtr<int> b(a);
    b.reset();
  }
  EXPECT_EQ(5, *a);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
