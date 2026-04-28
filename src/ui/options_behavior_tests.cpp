// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#define TEST_GUI
#include "tests/app_test.h"

#include "app/commands/options_behavior.h"

#include "ui/widget.h"

using namespace app;

TEST(OptionsBehavior, PixelGridControlsDisabledWhenGridIsHidden)
{
  ui::Widget pixelGridColorLabel;
  ui::Widget pixelGridColor;
  ui::Widget pixelGridOpacityLabel;
  ui::Widget pixelGridOpacity;
  ui::Widget pixelGridAutoOpacity;

  update_pixel_grid_options_enabled(false,
                                    &pixelGridColorLabel,
                                    &pixelGridColor,
                                    &pixelGridOpacityLabel,
                                    &pixelGridOpacity,
                                    &pixelGridAutoOpacity);

  EXPECT_FALSE(pixelGridColorLabel.isEnabled());
  EXPECT_FALSE(pixelGridColor.isEnabled());
  EXPECT_FALSE(pixelGridOpacityLabel.isEnabled());
  EXPECT_FALSE(pixelGridOpacity.isEnabled());
  EXPECT_FALSE(pixelGridAutoOpacity.isEnabled());
}

TEST(OptionsBehavior, PixelGridControlsEnabledWhenGridIsVisible)
{
  ui::Widget pixelGridColorLabel;
  ui::Widget pixelGridColor;
  ui::Widget pixelGridOpacityLabel;
  ui::Widget pixelGridOpacity;
  ui::Widget pixelGridAutoOpacity;

  update_pixel_grid_options_enabled(true,
                                    &pixelGridColorLabel,
                                    &pixelGridColor,
                                    &pixelGridOpacityLabel,
                                    &pixelGridOpacity,
                                    &pixelGridAutoOpacity);

  EXPECT_TRUE(pixelGridColorLabel.isEnabled());
  EXPECT_TRUE(pixelGridColor.isEnabled());
  EXPECT_TRUE(pixelGridOpacityLabel.isEnabled());
  EXPECT_TRUE(pixelGridOpacity.isEnabled());
  EXPECT_TRUE(pixelGridAutoOpacity.isEnabled());
}
