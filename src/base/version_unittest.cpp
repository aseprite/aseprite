// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/version.h"
#include "base/convert_to.h"

using namespace base;

std::ostream& operator<<(std::ostream& os, const Version& ver)
{
  return os << convert_to<string>(ver);
}

TEST(Version, Ctor)
{
  Version v0;
  EXPECT_EQ(0, v0[0]);
  EXPECT_EQ(0, v0[1]);
  EXPECT_EQ(0, v0[2]);
  EXPECT_EQ(0, v0[3]);
  EXPECT_EQ(0, v0[4]);

  Version v1(1);
  EXPECT_EQ(1, v1[0]);
  EXPECT_EQ(0, v1[1]);

  Version v2(1, 2);
  EXPECT_EQ(1, v2[0]);
  EXPECT_EQ(2, v2[1]);
  EXPECT_EQ(0, v2[2]);

  Version v3(1, 2, 3);
  EXPECT_EQ(1, v3[0]);
  EXPECT_EQ(2, v3[1]);
  EXPECT_EQ(3, v3[2]);
  EXPECT_EQ(0, v3[3]);

  Version v4(1, 2, 3, 4);
  EXPECT_EQ(1, v4[0]);
  EXPECT_EQ(2, v4[1]);
  EXPECT_EQ(3, v4[2]);
  EXPECT_EQ(4, v4[3]);
  EXPECT_EQ(0, v4[4]);
}

TEST(Version, StringToVersion)
{
  EXPECT_EQ(Version(), convert_to<Version>(string("")));
  EXPECT_EQ(Version(1), convert_to<Version>(string("1")));
  EXPECT_EQ(Version(1, 2), convert_to<Version>(string("1.2")));
  EXPECT_EQ(Version(1, 2, 3), convert_to<Version>(string("1.2.3")));
  EXPECT_EQ(Version(1, 2, 3, 4), convert_to<Version>(string("1.2.3.4")));
}

TEST(Version, VersionToString)
{
  EXPECT_EQ("", convert_to<string>(Version()));
  EXPECT_EQ("0", convert_to<string>(Version(0)));
  EXPECT_EQ("1", convert_to<string>(Version(1)));
  EXPECT_EQ("1.0", convert_to<string>(Version(1, 0)));
  EXPECT_EQ("0.0", convert_to<string>(Version(0, 0)));
  EXPECT_EQ("1.0.2", convert_to<string>(Version(1, 0, 2)));
}

TEST(Version, Equal)
{
  EXPECT_EQ(Version(), Version());
  EXPECT_EQ(Version(0), Version());
  EXPECT_EQ(Version(1), Version(1));
  EXPECT_EQ(Version(1), Version(1, 0));
  EXPECT_EQ(Version(1, 0), Version(1));
  EXPECT_EQ(Version(1, 2), Version(1, 2));
  EXPECT_EQ(Version(1, 2, 3), Version(1, 2, 3));
  EXPECT_EQ(Version(1, 0, 0), Version(1));
}

TEST(Version, NotEqual)
{
  EXPECT_FALSE(Version() != Version());
  EXPECT_TRUE(Version(1) != Version(1, 2));
  EXPECT_TRUE(Version() != Version(0, 1));
  EXPECT_TRUE(Version(1, 0) != Version(1, 0, 1));
  EXPECT_TRUE(Version(1, 2) != Version(1, 3));
  EXPECT_TRUE(Version(1, 2, 3) != Version(1, 2, 3, 4));
}

TEST(Version, LessThan)
{
  EXPECT_FALSE(Version() < Version(0));
  EXPECT_TRUE(Version(0) < Version(1));
  EXPECT_TRUE(Version(1, 2) < Version(1, 3));
  EXPECT_TRUE(Version(1, 2, 3) < Version(1, 2, 4));
  EXPECT_TRUE(Version(1, 2, 0, 4) < Version(1, 2, 3));
  EXPECT_FALSE(Version(1, 3) < Version(1, 2));
}

TEST(Version, AllComparisons)
{
  EXPECT_TRUE(Version(1, 2, 3) == Version(1, 2, 3));
  EXPECT_FALSE(Version(1, 2, 3) < Version(1, 2, 3));
  EXPECT_FALSE(Version(1, 2, 3) > Version(1, 2, 3));
  EXPECT_FALSE(Version(1, 2, 3) != Version(1, 2, 3));
  EXPECT_TRUE(Version(1, 2, 3) <= Version(1, 2, 3));
  EXPECT_TRUE(Version(1, 2, 3) >= Version(1, 2, 3));

  EXPECT_FALSE(Version(1, 2, 3) == Version(2));
  EXPECT_TRUE(Version(1, 2, 3) < Version(2));
  EXPECT_FALSE(Version(1, 2, 3) > Version(2));
  EXPECT_TRUE(Version(1, 2, 3) != Version(2));
  EXPECT_TRUE(Version(1, 2, 3) <= Version(2));
  EXPECT_FALSE(Version(1, 2, 3) >= Version(2));
}

TEST(Version, SimpleGetters)
{
  Version v0;
  EXPECT_EQ(0, v0.major());
  EXPECT_EQ(0, v0.minor());
  EXPECT_EQ(0, v0.revision());
  EXPECT_EQ(0, v0.build());

  Version v2(1, 2);
  EXPECT_EQ(1, v2.major());
  EXPECT_EQ(2, v2.minor());
  EXPECT_EQ(0, v2.revision());

  Version v4(1, 2, 3, 4);
  EXPECT_EQ(1, v4.major());
  EXPECT_EQ(2, v4.minor());
  EXPECT_EQ(3, v4.revision());
  EXPECT_EQ(4, v4.build());
}

TEST(Version, SimpleSetters)
{
  EXPECT_EQ(Version(1), Version().major(1));
  EXPECT_EQ(Version(1, 2), Version().major(1).minor(2));
  EXPECT_EQ(Version(1, 2), Version().minor(2).major(1));
  EXPECT_EQ(Version(1, 2, 3, 4), Version().major(1).minor(2).revision(3).build(4));
  EXPECT_EQ(Version(1, 2, 3, 4), Version().build(4).revision(3).minor(2).major(1));
  EXPECT_EQ(Version(0, 0, 0, 4), Version().build(4));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
