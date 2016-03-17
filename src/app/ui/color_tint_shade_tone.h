// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_COLOR_TINT_SHADE_TONE_H_INCLUDED
#define APP_UI_COLOR_TINT_SHADE_TONE_H_INCLUDED
#pragma once

#include "app/ui/color_selector.h"

namespace app {

  class ColorTintShadeTone : public ColorSelector {
  public:
    ColorTintShadeTone();

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;

  private:
    app::Color pickColor(const gfx::Point& pos) const;
    bool inHueBarArea(const gfx::Point& pos) const;
    int getHueBarSize() const;

    // True when the user pressed the mouse button in the hue slider.
    // It's used to avoid swapping in both areas (tint/shades/tones
    // area vs hue slider) when we drag the mouse above this widget.
    bool m_capturedInHue;
  };

} // namespace app

#endif
