// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_TINT_SHADE_TONE_H_INCLUDED
#define APP_UI_COLOR_TINT_SHADE_TONE_H_INCLUDED
#pragma once

#include "app/ui/color_selector.h"

namespace app {
  class Color;

  class ColorTintShadeTone : public ColorSelector {
  public:
    ColorTintShadeTone();

  protected:
    app::Color getMainAreaColor(const int u, const int umax,
                                const int v, const int vmax) override;
    app::Color getBottomBarColor(const int u, const int umax) override;
    void onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc) override;
    void onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc) override;
    void onPaintSurfaceInBgThread(os::Surface* s,
                                  const gfx::Rect& main,
                                  const gfx::Rect& bottom,
                                  const gfx::Rect& alpha,
                                  bool& stop) override;
    int onNeedsSurfaceRepaint(const app::Color& newColor) override;
  };

} // namespace app

#endif
