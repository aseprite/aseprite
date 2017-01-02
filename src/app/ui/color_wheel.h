// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_WHEEL_H_INCLUDED
#define APP_UI_COLOR_WHEEL_H_INCLUDED
#pragma once

#include "app/ui/color_selector.h"
#include "ui/button.h"

namespace app {

  class ColorWheel : public ColorSelector {
  public:
    enum class ColorModel {
      RGB,
      RYB,
      NORMAL_MAP,
    };

    enum class Harmony {
      NONE,
      COMPLEMENTARY,
      MONOCHROMATIC,
      ANALOGOUS,
      SPLIT,
      TRIADIC,
      TETRADIC,
      SQUARE,
      LAST = SQUARE
    };

    ColorWheel();

    // IColorSource
    app::Color getColorByPosition(const gfx::Point& pos) override;

    bool isDiscrete() const { return m_discrete; }
    void setDiscrete(bool state);

    void setColorModel(ColorModel colorModel);
    void setHarmony(Harmony harmony);

  private:
    app::Color getColorInClientPos(const gfx::Point& pos);
    void onResize(ui::ResizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    void onOptions();
    int getHarmonies() const;
    app::Color getColorInHarmony(int i) const;

    // Converts an hue angle from HSV <-> current color model hue.
    // With dir == +1, the angle is from the color model and it's converted to HSV hue.
    // With dir == -1, the angle came from HSV and is converted to the current color model.
    int convertHueAngle(int angle, int dir) const;

    gfx::Rect m_clientBounds;
    gfx::Rect m_wheelBounds;
    int m_wheelRadius;
    bool m_discrete;
    ColorModel m_colorModel;
    Harmony m_harmony;
    ui::ButtonBase m_options;

    // Internal flag used to know if after pickColor() we selected an
    // harmony.
    mutable bool m_harmonyPicked;
  };

} // namespace app

#endif
