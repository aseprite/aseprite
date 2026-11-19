// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#define TEST_APP
#include "tests/app_test.h"

#include "app/color.h"
#include "i18n/strings.h"

using namespace app;
using namespace doc;

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
  EXPECT_EQ(Color::fromGray(128, 255), Color::fromString("gray{128,255}"));
  EXPECT_EQ(Color::fromIndex(4), Color::fromString("index{4}"));
}

TEST(Color, toString)
{
  EXPECT_EQ("rgb{0,0,0,255}", Color::fromRgb(0, 0, 0).toString());
  EXPECT_EQ("rgb{32,16,255,255}", Color::fromRgb(32, 16, 255).toString());
  EXPECT_EQ("hsv{32.00,64.00,99.00,255}", Color::fromHsv(32, 64 / 100.0, 99 / 100.0).toString());
  EXPECT_EQ("hsl{32.00,64.00,99.00,255}", Color::fromHsl(32, 64 / 100.0, 99 / 100.0).toString());
  EXPECT_EQ("hsl{32.00,64.00,99.00,255}", Color::fromHsl(32, 64 / 100.0, 99 / 100.0).toString());
  EXPECT_EQ("gray{128,255}", Color::fromGray(128, 255).toString());
  EXPECT_EQ("index{4}", Color::fromIndex(4).toString());
}

TEST(Color, toLongHumanString)
{
  EXPECT_EQ("Mask", Color::fromMask().toLongHumanString(IMAGE_INDEXED));

  EXPECT_EQ("RGB 0 0 0 #000000", Color::fromRgb(0, 0, 0).toLongHumanString(IMAGE_RGB));
  EXPECT_EQ("RGB 255 0 0 #ff0000 A100",
            Color::fromRgb(255, 0, 0, 100).toLongHumanString(IMAGE_RGB));
  EXPECT_EQ("Gray 200 #c8c8c8", Color::fromRgb(200, 200, 200).toLongHumanString(IMAGE_GRAYSCALE));

  EXPECT_EQ("HSV 32° 64% 99% (RGB 252 177 91) #fcb15b",
            Color::fromHsv(32, 64 / 100.0, 99 / 100.0).toLongHumanString(IMAGE_RGB));
  EXPECT_EQ("Gray 171 #fcb15b",
            Color::fromHsv(32, 64 / 100.0, 99 / 100.0).toLongHumanString(IMAGE_GRAYSCALE));

  EXPECT_EQ("HSL 32° 64% 99% (RGB 254 253 251) #fefdfb",
            Color::fromHsl(32, 64 / 100.0, 99 / 100.0).toLongHumanString(IMAGE_RGB));
  EXPECT_EQ("HSL 32° 64% 99% (RGB 254 253 251) #fefdfb A80",
            Color::fromHsl(32, 64 / 100.0, 99 / 100.0, 80).toLongHumanString(IMAGE_RGB));
  EXPECT_EQ("Gray 252 #fefdfb A80",
            Color::fromHsl(32, 64 / 100.0, 99 / 100.0, 80).toLongHumanString(IMAGE_GRAYSCALE));

  EXPECT_EQ("Gray 100 #646464", Color::fromRgb(100, 100, 100).toLongHumanString(IMAGE_GRAYSCALE));
  EXPECT_EQ("Gray 128 #808080 A100", Color::fromGray(128, 100).toLongHumanString(IMAGE_GRAYSCALE));

  // First palette color of DB32
  EXPECT_EQ("Index 1 (RGB 34 32 52) #222034", Color::fromIndex(1).toLongHumanString(IMAGE_INDEXED));
  EXPECT_EQ("Index 256 (out of range) #000000 A0",
            Color::fromIndex(256).toLongHumanString(IMAGE_INDEXED));
}

TEST(Color, toShortHumanString)
{
  EXPECT_EQ("Mask", Color::fromMask().toShortHumanString(IMAGE_INDEXED));

  EXPECT_EQ("#000000", Color::fromRgb(0, 0, 0).toShortHumanString(IMAGE_RGB));
  EXPECT_EQ("#ff0000", Color::fromRgb(255, 0, 0, 100).toShortHumanString(IMAGE_RGB));
  EXPECT_EQ("Gry-200", Color::fromRgb(200, 200, 200).toShortHumanString(IMAGE_GRAYSCALE));

  EXPECT_EQ("32°64,99", Color::fromHsv(32, 64 / 100.0, 99 / 100.0).toShortHumanString(IMAGE_RGB));
  EXPECT_EQ("Gry-171",
            Color::fromHsv(32, 64 / 100.0, 99 / 100.0).toShortHumanString(IMAGE_GRAYSCALE));

  EXPECT_EQ("32°64,99", Color::fromHsl(32, 64 / 100.0, 99 / 100.0).toShortHumanString(IMAGE_RGB));
  EXPECT_EQ("32°64,99",
            Color::fromHsl(32, 64 / 100.0, 99 / 100.0, 80).toShortHumanString(IMAGE_RGB));
  EXPECT_EQ("Gry-252",
            Color::fromHsl(32, 64 / 100.0, 99 / 100.0, 80).toShortHumanString(IMAGE_GRAYSCALE));

  EXPECT_EQ("Gry-100", Color::fromRgb(100, 100, 100).toShortHumanString(IMAGE_GRAYSCALE));
  EXPECT_EQ("Gry-128", Color::fromGray(128, 100).toShortHumanString(IMAGE_GRAYSCALE));

  // First palette color of DB32
  EXPECT_EQ("Idx-1", Color::fromIndex(1).toShortHumanString(IMAGE_INDEXED));
  EXPECT_EQ("Idx-256", Color::fromIndex(256).toShortHumanString(IMAGE_INDEXED));
}
