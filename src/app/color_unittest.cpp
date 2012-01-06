/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtest/gtest.h>
#include "app/color.h"

inline std::ostream& operator<<(std::ostream& os, const Color& color) {
  return os << color.toString();
}

TEST(Color, fromRgb)
{
  EXPECT_EQ(32, Color::fromRgb(32, 16, 255).getRed());
  EXPECT_EQ(16, Color::fromRgb(32, 16, 255).getGreen());
  EXPECT_EQ(255, Color::fromRgb(32, 16, 255).getBlue());
}

TEST(Color, fromHsv)
{
  EXPECT_EQ(60, Color::fromHsv(60, 5, 100).getHue());
  EXPECT_EQ(5, Color::fromHsv(60, 5, 100).getSaturation());
  EXPECT_EQ(100, Color::fromHsv(60, 5, 100).getValue());
}

TEST(Color, fromString)
{
  EXPECT_EQ(Color::fromRgb(0, 0, 0), Color::fromString("rgb{0,0,0}"));
  EXPECT_EQ(Color::fromRgb(32, 16, 255), Color::fromString("rgb{32,16,255}"));
  EXPECT_EQ(Color::fromHsv(32, 64, 99), Color::fromString("hsv{32,64,99}"));
}

TEST(Color, toString)
{
  EXPECT_EQ("rgb{0,0,0}", Color::fromRgb(0, 0, 0).toString());
  EXPECT_EQ("rgb{32,16,255}", Color::fromRgb(32, 16, 255).toString());
  EXPECT_EQ("hsv{32,64,99}", Color::fromHsv(32, 64, 99).toString());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
