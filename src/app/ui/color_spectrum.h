// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_SPECTRUM_H_INCLUDED
#define APP_UI_COLOR_SPECTRUM_H_INCLUDED
#pragma once

#include "app/ui/color_selector.h"
#include "ui/button.h"

namespace app {

  class ColorSpectrum : public ColorSelector {
  public:
    ColorSpectrum();

    // IColorSource
    app::Color getColorByPosition(const gfx::Point& pos) override;

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
  };

} // namespace app

#endif
