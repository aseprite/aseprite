// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "gfx/clip.h"

using namespace gfx;

namespace gfx {

inline std::ostream& operator<<(std::ostream& os, const Clip& area)
{
  return os << "("
            << area.dst.x << ", "
            << area.dst.y << ", "
            << area.src.x << ", "
            << area.src.y << ", "
            << area.size.w << ", "
            << area.size.h << ")";
}

}

TEST(ScaledClip, WithoutClip)
{
  Clip area;

  area = Clip(0, 0, 0, 0, 16, 16);
  EXPECT_TRUE(area.clip(16, 16, 16, 16));
  EXPECT_EQ(Clip(0, 0, 0, 0, 16, 16), area);

  area = Clip(2, 2, 0, 0, 16, 16);
  EXPECT_TRUE(area.clip(32, 32, 16, 16));
  EXPECT_EQ(Clip(2, 2, 0, 0, 16, 16), area);
}

TEST(ScaledClip, FullyClipped)
{
  Clip area;

  area = Clip(32, 32, 0, 0, 16, 16);
  EXPECT_FALSE(area.clip(32, 32, 16, 16));

  area = Clip(-16, -16, 0, 0, 16, 16);
  EXPECT_FALSE(area.clip(32, 32, 16, 16));

  area = Clip(0, 0, 16, 16, 16, 16);
  EXPECT_FALSE(area.clip(32, 32, 16, 16));
}

TEST(ScaledClip, WithoutZoomWithClip)
{
  Clip area;

  area = Clip(2, 3, 1, -1, 4, 3);
  EXPECT_TRUE(area.clip(30, 29, 16, 16));
  EXPECT_EQ(Clip(2, 4, 1, 0, 4, 2), area);

  area = Clip(0, 0, -1, -4, 8, 5);
  EXPECT_TRUE(area.clip(3, 32, 8, 8));
  EXPECT_EQ(Clip(1, 4, 0, 0, 2, 1), area);
}

TEST(ScaledClip, Zoom)
{
  Clip area;

  area = Clip(0, 0, 0, 0, 32, 32);
  EXPECT_TRUE(area.clip(32, 32, 16, 16));
  EXPECT_EQ(Clip(0, 0, 0, 0, 16, 16), area);

  area = Clip(0, 0, 1, 2, 32, 32);
  EXPECT_TRUE(area.clip(32, 32, 32, 32));
  EXPECT_EQ(Clip(0, 0, 1, 2, 31, 30), area);

  // X:
  //  -1 0 1 2 3 4 5
  // [        ]
  //   a[a a b] b c c c          DST
  // a a[a b b]b c c c           SRC
  //
  // Y:
  //  -1 0 1 2 3
  //      [       ]
  //       a[a a]b b b           DST
  //        [a a]a b b b c c     SRC
  area = Clip(-1, 1, 1, -1, 4, 4);
  EXPECT_TRUE(area.clip(6, 4, 9, 9));
  EXPECT_EQ(Clip(0, 2, 2, 0, 3, 2), area);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
