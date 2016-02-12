// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "tests/test.h"

#include "app/color.h"

using namespace app;

namespace app {

  inline std::ostream& operator<<(std::ostream& os, const Color& color) {
    return os << color.toString();
  }

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
  EXPECT_EQ(Color::fromRgb(0, 0, 0), Color::fromString("rgb{0,0.0,0}"));
  EXPECT_EQ(Color::fromRgb(32, 16, 255), Color::fromString("rgb{32,16,255}"));
  EXPECT_EQ(Color::fromHsv(32, 64, 99), Color::fromString("hsv{32,64,99}"));
}

TEST(Color, toString)
{
  EXPECT_EQ("rgb{0,0,0,255}", Color::fromRgb(0, 0, 0).toString());
  EXPECT_EQ("rgb{32,16,255,255}", Color::fromRgb(32, 16, 255).toString());
  EXPECT_EQ("hsv{32.00,64.00,99.00,255}", Color::fromHsv(32, 64, 99).toString());
}
