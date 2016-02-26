// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_COLOR_TINT_SHADE_TONE_H_INCLUDED
#define APP_UI_COLOR_TINT_SHADE_TONE_H_INCLUDED
#pragma once

#include "app/color.h"
#include "base/signal.h"
#include "ui/mouse_buttons.h"
#include "ui/widget.h"

namespace app {

  class ColorTintShadeTone : public ui::Widget {
  public:
    ColorTintShadeTone();
    ~ColorTintShadeTone();

    void selectColor(const app::Color& color);

    // Signals
    base::Signal2<void, const app::Color&, ui::MouseButtons> ColorChange;

  protected:
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;

  private:
    app::Color pickColor(const gfx::Point& pos) const;
    bool inHueBarArea(const gfx::Point& pos) const;
    int getHueBarSize() const;

    app::Color m_color;

    // True when the user pressed the mouse button in the hue slider.
    // It's used to avoid swapping in both areas (tint/shades/tones
    // area vs hue slider) when we drag the mouse above this widget.
    bool m_capturedInHue;
  };

} // namespace app

#endif
