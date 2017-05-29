// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_SPECTRUM_H_INCLUDED
#define APP_UI_COLOR_SPECTRUM_H_INCLUDED
#pragma once

#include "app/ui/color_selector.h"

namespace app {

  class ColorSpectrum : public ColorSelector {
  public:
    ColorSpectrum();

  protected:
    app::Color getMainAreaColor(const int u, const int umax,
                                const int v, const int vmax) override;
    app::Color getBottomBarColor(const int u, const int umax) override;
    void onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc) override;
    void onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc) override;
  };

} // namespace app

#endif
