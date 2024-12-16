// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/app_test.h"

#include "app/color.h"

using namespace app;

namespace app {

inline std::ostream& operator<<(std::ostream& os, const Color& color)
{
  return os << color.toString();
}

} // namespace app

TEST(Color, fromRgb)
{
  EXPECT_EQ(32, Color::fromRgb(32, 16, 255).getRed());
  EXPECT_EQ(16, Color::fromRgb(32, 16, 255).getGreen());
  EXPECT_EQ(255, Color::fromRgb(32, 16, 255).getBlue());
}

TEST(Color, fromHsv)
{
  EXPECT_EQ(60.0, Color::fromHsv(60, 0.05, 1.0).getHsvHue());
  EXPECT_EQ(0.05, Color::fromHsv(60, 0.05, 1.0).getHsvSaturation());
  EXPECT_EQ(1.00, Color::fromHsv(60, 0.05, 1.0).getHsvValue());
}

TEST(Color, fromHsl)
{
  EXPECT_EQ(60.0, Color::fromHsl(60, 0.05, 1.0).getHslHue());
  EXPECT_EQ(0.05, Color::fromHsl(60, 0.05, 1.0).getHslSaturation());
  EXPECT_EQ(1.00, Color::fromHsl(60, 0.05, 1.0).getHslLightness());
}

TEST(Color, fromString)
{
  EXPECT_EQ(Color::fromRgb(0, 0, 0), Color::fromString("rgb{0,0.0,0}"));
  EXPECT_EQ(Color::fromRgb(32, 16, 255), Color::fromString("rgb{32,16,255}"));
  EXPECT_EQ(Color::fromHsv(32, 0.64, 0.99), Color::fromString("hsv{32,64,99}"));
  EXPECT_EQ(Color::fromHsl(32, 0.64, 0.99), Color::fromString("hsl{32,64,99}"));
}

TEST(Color, toString)
{
  EXPECT_EQ("rgb{0,0,0,255}", Color::fromRgb(0, 0, 0).toString());
  EXPECT_EQ("rgb{32,16,255,255}", Color::fromRgb(32, 16, 255).toString());
  EXPECT_EQ("hsv{32.00,64.00,99.00,255}", Color::fromHsv(32, 64 / 100.0, 99 / 100.0).toString());
  EXPECT_EQ("hsl{32.00,64.00,99.00,255}", Color::fromHsl(32, 64 / 100.0, 99 / 100.0).toString());
}
