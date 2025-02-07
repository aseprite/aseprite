// Aseprite Document Library
// Copyright (c) 2020-2022 Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/palette.h"
#include "doc/palette_picks.h"
#include "doc/remap.h"

using namespace doc;

static PalettePicks make(int n, const std::vector<int>& v)
{
  PalettePicks picks(n);
  std::fill(picks.begin(), picks.end(), false);
  for (int i = 0; i < int(v.size()); ++i)
    picks[v[i]] = true;
  return picks;
}

static void expect_map(const Remap& map, const std::vector<int>& expected)
{
  EXPECT_EQ(map.size(), expected.size());
  for (int i = 0; i < int(map.size()); ++i) {
    EXPECT_EQ(expected[i], map[i]) << " When i=" << i;
  }
}

TEST(Remap, ToMovePicks)
{
  auto entries = make(20, { 6, 7, 14 });
  Remap map = create_remap_to_move_picks(entries, 1);
  //                0  1  2  3  4  5  6  7  8   9  10  11  12  13 14  15  16  17  18  19
  expect_map(map, { 0, 4, 5, 6, 7, 8, 1, 2, 9, 10, 11, 12, 13, 14, 3, 15, 16, 17, 18, 19 });

  map = create_remap_to_move_picks(entries, 18);
  //                0  1  2  3  4  5   6   7  8  9 10 11  12  13  14  15  16  17  18  19
  expect_map(map, { 0, 1, 2, 3, 4, 5, 15, 16, 6, 7, 8, 9, 10, 11, 17, 12, 13, 14, 18, 19 });

  PalettePicks all(map.size());
  all.all();
  EXPECT_TRUE(map.isInvertible(all));
}

TEST(Remap, ToExpandPalette)
{
  Remap map = create_remap_to_expand_palette(10, 3, 1);

  EXPECT_EQ(0, map[0]);
  EXPECT_EQ(4, map[1]);
  EXPECT_EQ(5, map[2]);
  EXPECT_EQ(6, map[3]);
  EXPECT_EQ(7, map[4]);
  EXPECT_EQ(8, map[5]);
  EXPECT_EQ(9, map[6]);
  EXPECT_EQ(1, map[7]);
  EXPECT_EQ(2, map[8]);
  EXPECT_EQ(3, map[9]);

  map = create_remap_to_expand_palette(10, 3, 8);

  EXPECT_EQ(0, map[0]);
  EXPECT_EQ(1, map[1]);
  EXPECT_EQ(2, map[2]);
  EXPECT_EQ(3, map[3]);
  EXPECT_EQ(4, map[4]);
  EXPECT_EQ(5, map[5]);
  EXPECT_EQ(6, map[6]);
  EXPECT_EQ(7, map[7]);
  EXPECT_EQ(8, map[8]);
  EXPECT_EQ(9, map[9]);

  PalettePicks all(map.size());
  all.all();
  EXPECT_TRUE(map.isInvertible(all));
}

TEST(Remap, BetweenPalettesChangeMask)
{
  Palette a(frame_t(0), 4);
  Palette b(frame_t(0), 4);

  a.setEntry(0, rgba(255, 0, 0, 255));
  a.setEntry(1, rgba(0, 255, 0, 255));
  a.setEntry(2, rgba(0, 0, 255, 255));
  a.setEntry(3, rgba(255, 255, 255, 255));

  b.setEntry(0, rgba(255, 255, 255, 255));
  b.setEntry(1, rgba(0, 255, 0, 255));
  b.setEntry(2, rgba(0, 0, 255, 255));
  b.setEntry(3, rgba(255, 0, 0, 255));

  Remap map = create_remap_to_change_palette(&a, &b, 0, true);

  EXPECT_EQ(3, map[0]);
  EXPECT_EQ(1, map[1]);
  EXPECT_EQ(2, map[2]);
  EXPECT_EQ(0, map[3]);

  PalettePicks all(map.size());
  all.all();
  EXPECT_TRUE(map.isInvertible(all));
}

TEST(Remap, BetweenPalettesDontChangeMask)
{
  Palette a(frame_t(0), 4);
  Palette b(frame_t(0), 4);

  a.setEntry(0, rgba(0, 0, 0, 255));
  a.setEntry(1, rgba(0, 255, 255, 255));
  a.setEntry(2, rgba(0, 0, 0, 255));
  a.setEntry(3, rgba(255, 0, 0, 255));

  b.setEntry(0, rgba(255, 0, 0, 255));
  b.setEntry(1, rgba(0, 255, 255, 255));
  b.setEntry(2, rgba(0, 0, 0, 255));
  b.setEntry(3, rgba(0, 0, 0, 255));

  Remap map = create_remap_to_change_palette(&a, &b, 2, true);

  EXPECT_EQ(3, map[0]);
  EXPECT_EQ(1, map[1]);
  EXPECT_EQ(2, map[2]);
  EXPECT_EQ(0, map[3]);

  PalettePicks all(map.size());
  all.all();
  EXPECT_TRUE(map.isInvertible(all));
}

TEST(Remap, BetweenPalettesDontChangeMaskForced)
{
  Palette a(frame_t(0), 4);
  Palette b(frame_t(0), 4);

  a.setEntry(0, rgba(0, 0, 0, 255));
  a.setEntry(1, rgba(0, 255, 255, 255));
  a.setEntry(2, rgba(0, 0, 0, 255));
  a.setEntry(3, rgba(255, 0, 0, 255));

  b.setEntry(0, rgba(255, 0, 0, 255));
  b.setEntry(1, rgba(0, 255, 255, 255));
  b.setEntry(2, rgba(0, 0, 0, 255));
  b.setEntry(3, rgba(0, 0, 0, 255));

  Remap map = create_remap_to_change_palette(&a, &b, 2, false);

  EXPECT_EQ(3, map[0]);
  EXPECT_EQ(1, map[1]);
  EXPECT_EQ(2, map[2]);
  EXPECT_EQ(0, map[3]);

  PalettePicks all(map.size());
  all.all();
  EXPECT_TRUE(map.isInvertible(all));
}

TEST(Remap, BetweenPalettesNonInvertible)
{
  // create_remap_to_change_palette() uses findBestfit()
  doc::Palette::initBestfit();

  Palette a(frame_t(0), 4);
  Palette b(frame_t(0), 3);

  a.setEntry(0, rgba(0, 0, 0, 255));
  a.setEntry(1, rgba(64, 0, 0, 255));
  a.setEntry(2, rgba(0, 255, 0, 255));
  a.setEntry(3, rgba(0, 0, 128, 255));

  b.setEntry(0, rgba(0, 255, 0, 255));
  b.setEntry(1, rgba(0, 0, 0, 255));
  b.setEntry(2, rgba(64, 0, 0, 255));

  Remap map = create_remap_to_change_palette(&a, &b, 0, true);

  EXPECT_EQ(1, map[0]);
  EXPECT_EQ(2, map[1]);
  EXPECT_EQ(0, map[2]);
  EXPECT_EQ(2, map[3]);

  PalettePicks all(map.size());
  all.all();
  EXPECT_FALSE(map.isInvertible(all));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
