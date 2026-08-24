// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "app/app.h"
#include "app/cli/app_options.h"
#include "app/ui/color_wheel.h"
#include "os/system.h"

using namespace app;

class TestColorWheel : public ColorWheel {
public:
  app::Color getMainAreaColor(const int u, const int umax, const int v, const int vmax) override
  {
    return ColorWheel::getMainAreaColor(u, umax, v, vmax);
  }
};

TEST(ColorWheel, NormalMapDiscreteCenter)
{
  TestColorWheel wheel;
  wheel.setColorModel(ColorWheel::ColorModel::NORMAL_MAP);
  wheel.setDiscrete(true);

  // Center pixel: u = 50, umax = 100, v = 50, vmax = 100 -> (u - umax/2 == 0, v - vmax/2 == 0)
  const app::Color centerColor = wheel.getMainAreaColor(50, 100, 50, 100);
  EXPECT_EQ(128, centerColor.getRed());
  EXPECT_EQ(128, centerColor.getGreen());
  EXPECT_EQ(255, centerColor.getBlue());
}

TEST(ColorWheel, NormalMapDiscreteRings)
{
  TestColorWheel wheel;
  wheel.setColorModel(ColorWheel::ColorModel::NORMAL_MAP);
  wheel.setDiscrete(true);

  // Wheel radius with size 100 is around 50 (umax = 100, vmax = 100).
  // Center is at (50, 50).
  const app::Color c0 = wheel.getMainAreaColor(50, 100, 50, 100);  // d = 0 (Center)
  const app::Color c1 = wheel.getMainAreaColor(60, 100, 50, 100);  // d = 10 (Ring 1, nd = 0.2)
  const app::Color c2 = wheel.getMainAreaColor(70, 100, 50, 100);  // d = 20 (Ring 2, nd = 0.4)
  const app::Color c3 = wheel.getMainAreaColor(80, 100, 50, 100);  // d = 30 (Ring 3, nd = 0.6)
  const app::Color c4 = wheel.getMainAreaColor(90, 100, 50, 100);  // d = 40 (Ring 4, nd = 0.8)
  const app::Color c5 = wheel.getMainAreaColor(100, 100, 50, 100); // d = 50 (Ring 5, nd = 1.0)

  EXPECT_EQ(255, c0.getBlue());
  EXPECT_EQ(249, c1.getBlue());
  EXPECT_EQ(231, c2.getBlue());
  EXPECT_EQ(203, c3.getBlue());
  EXPECT_EQ(167, c4.getBlue());
  EXPECT_EQ(128, c5.getBlue());

  // Verify blue channel monotonically decreases from center to edge
  EXPECT_GT(c0.getBlue(), c1.getBlue());
  EXPECT_GT(c1.getBlue(), c2.getBlue());
  EXPECT_GT(c2.getBlue(), c3.getBlue());
  EXPECT_GT(c3.getBlue(), c4.getBlue());
  EXPECT_GT(c4.getBlue(), c5.getBlue());
}

TEST(ColorWheel, NormalMapContinuousCenter)
{
  TestColorWheel wheel;
  wheel.setColorModel(ColorWheel::ColorModel::NORMAL_MAP);
  wheel.setDiscrete(false);

  const app::Color centerColor = wheel.getMainAreaColor(50, 100, 50, 100);
  EXPECT_EQ(128, centerColor.getRed());
  EXPECT_EQ(128, centerColor.getGreen());
  EXPECT_EQ(255, centerColor.getBlue());
}

int app_main(int argc, char* argv[])
{
  os::SystemRef system = os::System::make();
  const char* argv2[] = { argv[0] };
  const app::AppOptions options(sizeof(argv2) / sizeof(argv2[0]), argv2);
  app::App app;
  app.initialize(options);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
