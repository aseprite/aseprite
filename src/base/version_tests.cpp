// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/version.h"
#include "base/convert_to.h"

using namespace base;

namespace base {

  std::ostream& operator<<(std::ostream& os, const Version& ver) {
    return os << ver.str();
  }

}

TEST(Version, Ctor)
{
  EXPECT_EQ("1", Version("1").str());
  EXPECT_EQ("1.2", Version("1.2").str());
  EXPECT_EQ("1.2-rc3", Version("1.2-rc3").str());
  EXPECT_EQ("1.2-beta3", Version("1.2-beta3").str());
  EXPECT_EQ("1.2-beta", Version("1.2-beta").str());
  EXPECT_EQ("1.2-beta", Version("1.2-beta0").str());
}

TEST(Version, LessThan)
{
  EXPECT_TRUE(Version("0") < Version("1"));
  EXPECT_TRUE(Version("1.2") < Version("1.3"));
  EXPECT_TRUE(Version("1.2.3") < Version("1.2.4"));
  EXPECT_TRUE(Version("1.2.0.4") < Version("1.2.3"));
  EXPECT_TRUE(Version("1.3-dev") < Version("1.3"));
  EXPECT_TRUE(Version("1.3-dev") < Version("1.4"));
  EXPECT_TRUE(Version("1.1-beta") < Version("1.1-beta1"));
  EXPECT_TRUE(Version("1.1-beta1") < Version("1.1-beta2"));
  EXPECT_TRUE(Version("1.1-beta2") < Version("1.1-rc1"));

  EXPECT_FALSE(Version("1") < Version("0"));
  EXPECT_FALSE(Version("1.3") < Version("1.2"));
  EXPECT_FALSE(Version("1.2.4") < Version("1.2.3"));
  EXPECT_FALSE(Version("1.2.3") < Version("1.2.0.4"));
  EXPECT_FALSE(Version("1.3") < Version("1.3-dev"));
  EXPECT_FALSE(Version("1.4") < Version("1.3-dev"));
  EXPECT_FALSE(Version("1.1-beta1") < Version("1.1-beta"));
  EXPECT_FALSE(Version("1.1-beta2") < Version("1.1-beta1"));
  EXPECT_FALSE(Version("1.1-rc1") < Version("1.1-beta2"));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
