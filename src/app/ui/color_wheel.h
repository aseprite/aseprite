// Aseprite
// Copyright (C) 2001-2017  David Capello
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

    bool isDiscrete() const { return m_discrete; }
    void setDiscrete(bool state);

    void setColorModel(ColorModel colorModel);
    void setHarmony(Harmony harmony);

  protected:
    app::Color getMainAreaColor(const int u, const int umax,
                                const int v, const int vmax) override;
    app::Color getBottomBarColor(const int u, const int umax) override;
    void onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc) override;
    void onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc) override;

  private:
    void onResize(ui::ResizeEvent& ev) override;
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
    ui::Button m_options;

    // Internal flag used to know if after pickColor() we selected an
    // harmony.
    mutable bool m_harmonyPicked;
  };

} // namespace app

#endif
