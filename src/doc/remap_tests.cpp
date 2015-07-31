// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/remap.h"
#include "doc/palette.h"
#include "doc/palette_picks.h"

using namespace doc;

TEST(Remap, ToMovePicks)
{
  PalettePicks entries(20);
  std::fill(entries.begin(), entries.end(), false);
  entries[6] =
    entries[7] =
    entries[14] = true;

  Remap map = create_remap_to_move_picks(entries, 1);

  EXPECT_EQ(0, map[0]);
  EXPECT_EQ(4, map[1]);
  EXPECT_EQ(5, map[2]);
  EXPECT_EQ(6, map[3]);
  EXPECT_EQ(7, map[4]);
  EXPECT_EQ(8, map[5]);
  EXPECT_EQ(1, map[6]);
  EXPECT_EQ(2, map[7]);
  EXPECT_EQ(9, map[8]);
  EXPECT_EQ(10, map[9]);
  EXPECT_EQ(11, map[10]);
  EXPECT_EQ(12, map[11]);
  EXPECT_EQ(13, map[12]);
  EXPECT_EQ(14, map[13]);
  EXPECT_EQ(3, map[14]);
  EXPECT_EQ(15, map[15]);
  EXPECT_EQ(16, map[16]);
  EXPECT_EQ(17, map[17]);
  EXPECT_EQ(18, map[18]);
  EXPECT_EQ(19, map[19]);

  map = create_remap_to_move_picks(entries, 18);

  EXPECT_EQ(0, map[0]);
  EXPECT_EQ(1, map[1]);
  EXPECT_EQ(2, map[2]);
  EXPECT_EQ(3, map[3]);
  EXPECT_EQ(4, map[4]);
  EXPECT_EQ(5, map[5]);
  EXPECT_EQ(15, map[6]);
  EXPECT_EQ(16, map[7]);
  EXPECT_EQ(6, map[8]);
  EXPECT_EQ(7, map[9]);
  EXPECT_EQ(8, map[10]);
  EXPECT_EQ(9, map[11]);
  EXPECT_EQ(10, map[12]);
  EXPECT_EQ(11, map[13]);
  EXPECT_EQ(17, map[14]);
  EXPECT_EQ(12, map[15]);
  EXPECT_EQ(13, map[16]);
  EXPECT_EQ(14, map[17]);
  EXPECT_EQ(18, map[18]);
  EXPECT_EQ(19, map[19]);

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
