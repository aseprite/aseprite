// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_COLOR_SPECTRUM_H_INCLUDED
#define APP_UI_COLOR_SPECTRUM_H_INCLUDED
#pragma once

#include "app/ui/color_selector.h"
#include "ui/button.h"

namespace app {

  class ColorSpectrum : public ColorSelector {
  public:
    ColorSpectrum();

    app::Color pickColor(const gfx::Point& pos) const;

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
  };

} // namespace app

#endif
