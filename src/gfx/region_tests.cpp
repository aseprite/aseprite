// Aseprite Gfx Library
// Copyright (C) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "gfx/point.h"
#include "gfx/rect_io.h"
#include "gfx/region.h"

using namespace std;
using namespace gfx;

ostream& operator<<(ostream& os, const Region& rgn)
{
  os << "{";
  for (Region::const_iterator it=rgn.begin(), end=rgn.end();
       it != end; ) {
    os << *it;
    ++it;
    if (it != end)
      os << ", ";
  }
  os << "}";
  return os;
}

TEST(Region, Ctor)
{
  EXPECT_EQ(0, Region().size());
  EXPECT_TRUE(Region(Rect(0, 0, 0, 0)).isEmpty());
  EXPECT_TRUE(Region().isEmpty());
  ASSERT_EQ(0, Region(Rect(0, 0, 0, 0)).size());
  ASSERT_EQ(1, Region(Rect(0, 0, 1, 1)).size());
  EXPECT_EQ(Rect(2, 3, 4, 5), Region(Rect(2, 3, 4, 5))[0]);
}

TEST(Region, Equal)
{
  Region a;
  a = Rect(2, 3, 4, 5);
  EXPECT_EQ(Rect(2, 3, 4, 5), a.bounds());
  EXPECT_EQ(Rect(2, 3, 4, 5), a[0]);
  EXPECT_FALSE(a.isEmpty());

  a = Rect(6, 7, 8, 9);
  EXPECT_EQ(Rect(6, 7, 8, 9), a.bounds());
  EXPECT_EQ(Rect(6, 7, 8, 9), a[0]);

  Region b;
  b = a;
  EXPECT_EQ(Rect(6, 7, 8, 9), b[0]);

  b = Rect(0, 0, 0, 0);
  EXPECT_TRUE(b.isEmpty());
}

TEST(Region, Clear)
{
  Region a(Rect(2, 3, 4, 5));
  EXPECT_FALSE(a.isEmpty());
  a.clear();
  EXPECT_TRUE(a.isEmpty());
}

TEST(Region, Union)
{
  Region a(Rect(2, 3, 4, 5));
  Region b(Rect(6, 3, 4, 5));
  EXPECT_EQ(Rect(2, 3, 8, 5), Region().createUnion(a, b).bounds());
  EXPECT_EQ(Rect(2, 3, 8, 5), Region().createUnion(b, a).bounds());
  ASSERT_EQ(1, Region().createUnion(a, b).size());
  ASSERT_EQ(1, Region().createUnion(b, a).size());
  EXPECT_EQ(Rect(2, 3, 8, 5), Region().createUnion(a, b)[0]);
  EXPECT_EQ(Rect(2, 3, 8, 5), Region().createUnion(b, a)[0]);
}

TEST(Region, ContainsPoint)
{
  Region a(Rect(2, 3, 4, 5));
  EXPECT_TRUE(a.contains(Point(2, 3)));
  EXPECT_FALSE(a.contains(Point(2-1, 3-1)));
  EXPECT_FALSE(a.contains(Point(2+4, 3)));
  EXPECT_FALSE(a.contains(Point(2, 3+5)));
  EXPECT_FALSE(a.contains(Point(2+4, 3+5)));
  EXPECT_TRUE(a.contains(Point(2+4-1, 3)));
  EXPECT_TRUE(a.contains(Point(2, 3+5-1)));
  EXPECT_TRUE(a.contains(Point(2+4-1, 3+5-1)));
}

TEST(Region, Iterators)
{
  Region a;
  a.createUnion(a, Region(Rect(0, 0, 32, 64)));
  a.createUnion(a, Region(Rect(0, 0, 64, 32)));
  int c = 0;
  for (Region::iterator it=a.begin(), end=a.end(); it!=end; ++it) {
    ++c;
  }
  EXPECT_EQ(2, c);

  c = 0;
  for (Region::const_iterator it=a.begin(), end=a.end(); it!=end; ++it) {
    ++c;
  }
  EXPECT_EQ(2, c);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
