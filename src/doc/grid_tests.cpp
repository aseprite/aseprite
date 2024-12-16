// Aseprite Document Library
// Copyright (c) 2019-2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/grid.h"
#include "doc/mask.h"
#include "doc/util.h"
#include "gfx/rect_io.h"
#include "gfx/region.h"

// TODO add gfx/point_io.h and gfx/size_io.h
namespace gfx {

inline std::ostream& operator<<(std::ostream& os, const Point& pt)
{
  return os << "(" << pt.x << ", " << pt.y << ")";
}

inline std::ostream& operator<<(std::ostream& os, const Size& sz)
{
  return os << "(" << sz.w << ", " << sz.h << ")";
}

} // namespace gfx

using namespace doc;
using namespace gfx;

TEST(Grid, Rect)
{
  auto grid = Grid::MakeRect(Size(16, 16));

  EXPECT_EQ(Size(16, 16), grid.tileSize());
  EXPECT_EQ(Point(16, 16), grid.tileOffset());

  EXPECT_EQ(Point(16, 16), grid.tileToCanvas(Point(1, 1)));
  EXPECT_EQ(Point(32, 16), grid.tileToCanvas(Point(2, 1)));
  EXPECT_EQ(Point(0, 0), grid.canvasToTile(Point(5, 5)));
  EXPECT_EQ(Point(1, 1), grid.canvasToTile(Point(16, 16)));
  EXPECT_EQ(Rect(1, 0, 1, 2), grid.canvasToTile(Rect(16, 5, 16, 16)));

  EXPECT_EQ(Rect(0, 0, 32, 32), grid.alignBounds(Rect(4, 5, 16, 17)));
  EXPECT_EQ(Rect(16, 16, 16, 16), grid.alignBounds(Rect(16, 16, 16, 16)));
  EXPECT_EQ(Rect(16, 16, 32, 16), grid.alignBounds(Rect(16, 16, 17, 16)));
  EXPECT_EQ(Rect(16, 16, 48, 16), grid.alignBounds(Rect(17, 16, 32, 16)));

  gfx::Region rgn;
  rgn |= gfx::Region(gfx::Rect(5, 17, 8, 8));
  rgn |= gfx::Region(gfx::Rect(17, 5, 16, 8));
  auto pts = grid.tilesInCanvasRegion(rgn);
  ASSERT_EQ(3, pts.size());
  EXPECT_EQ(gfx::Point(1, 0), pts[0]);
  EXPECT_EQ(gfx::Point(2, 0), pts[1]);
  EXPECT_EQ(gfx::Point(0, 1), pts[2]);
}

TEST(Grid, RectWithOffset)
{
  auto grid = Grid::MakeRect(Size(16, 16));
  grid.origin(gfx::Point(17, 16));

  EXPECT_EQ(Point(0, 0), grid.canvasToTile(Point(17, 16)));
  EXPECT_EQ(Point(0, 0), grid.canvasToTile(Point(17 + 15, 16)));
  EXPECT_EQ(Point(1, 0), grid.canvasToTile(Point(17 + 16, 16)));

  EXPECT_EQ(Point(-1, 0), grid.canvasToTile(Point(16, 16)));
  EXPECT_EQ(Point(-1, 0), grid.canvasToTile(Point(2, 16)));
  EXPECT_EQ(Point(-1, 0), grid.canvasToTile(Point(1, 16)));

  grid.origin(gfx::Point(-1, -1));
  EXPECT_EQ(Rect(1, 1, 1, 1), grid.canvasToTile(Rect(30, 30, 1, 1)));
}

TEST(Grid, MakeAlignedMask)
{
  auto grid = Grid::MakeRect(Size(4, 4));
  grid.origin(gfx::Point(1, 1));
  auto mask = Mask();
  mask.replace(gfx::Rect(3, 3, 4, 4));
  auto gridAlignedMask = make_aligned_mask(&grid, &mask);
  EXPECT_EQ(gfx::Rect(1, 1, 8, 8), gridAlignedMask.bounds());

  mask.replace(gfx::Rect(1, 1, 4, 4));
  auto gridAlignedMask2 = make_aligned_mask(&grid, &mask);
  EXPECT_EQ(gfx::Rect(1, 1, 4, 4), gridAlignedMask2.bounds());

  mask.add(gfx::Rect(8, 4, 1, 1));
  mask.add(gfx::Rect(7, 7, 1, 1));
  auto gridAlignedMask3 = make_aligned_mask(&grid, &mask);
  const bool _ = false;
  const bool X = true;
  bool expected[8 * 8] = {
    X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,

    _, _, _, _, X, X, X, X, _, _, _, _, X, X, X, X, _, _, _, _, X, X, X, X, _, _, _, _, X, X, X, X,
  };
  int c = 0;
  for (int j = 0; j < 8; ++j)
    for (int i = 0; i < 8; ++i)
      EXPECT_EQ(expected[c++], gridAlignedMask3.bitmap()->getPixel(i, j));

  mask.replace(gfx::Rect(4, 4, 1, 1));
  mask.add(gfx::Rect(5, 5, 1, 1));
  mask.add(gfx::Rect(8, 4, 1, 1));
  mask.add(gfx::Rect(9, 1, 1, 1));
  auto gridAlignedMask4 = make_aligned_mask(&grid, &mask);
  bool expected2[12 * 8] = {
    X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
    X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,

    _, _, _, _, X, X, X, X, _, _, _, _, _, _, _, _, X, X, X, X, _, _, _, _,
    _, _, _, _, X, X, X, X, _, _, _, _, _, _, _, _, X, X, X, X, _, _, _, _,
  };
  c = 0;
  for (int j = 0; j < 8; ++j)
    for (int i = 0; i < 12; ++i)
      EXPECT_EQ(expected2[c++], gridAlignedMask4.bitmap()->getPixel(i, j));

  grid.origin(gfx::Point(2, 1));
  auto gridAlignedMask5 = make_aligned_mask(&grid, &mask);
  bool expected3[8 * 8] = {
    X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,

    X, X, X, X, _, _, _, _, X, X, X, X, _, _, _, _, X, X, X, X, _, _, _, _, X, X, X, X, _, _, _, _,
  };
  c = 0;
  for (int j = 0; j < 8; ++j)
    for (int i = 0; i < 8; ++i)
      EXPECT_EQ(expected3[c++], gridAlignedMask5.bitmap()->getPixel(i, j));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
