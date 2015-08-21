// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_COLOR_WHEEL_H_INCLUDED
#define APP_UI_COLOR_WHEEL_H_INCLUDED
#pragma once

#include "app/color.h"
#include "base/signal.h"
#include "ui/button.h"
#include "ui/mouse_buttons.h"
#include "ui/widget.h"

namespace app {

  class ColorWheel : public ui::Widget {
  public:
    ColorWheel();
    ~ColorWheel();

    app::Color pickColor(const gfx::Point& pos) const;
    void selectColor(const app::Color& color);

    bool isDiscrete() const { return m_discrete; }
    void setDiscrete(bool state);

    // Signals
    Signal2<void, const app::Color&, ui::MouseButtons> ColorChange;

  private:
    void onPreferredSize(ui::PreferredSizeEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    void onOptions();

    gfx::Rect m_clientBounds;
    gfx::Rect m_wheelBounds;
    int m_wheelRadius;
    bool m_discrete;
    ui::ButtonBase m_options;
    app::Color m_mainColor;
  };

} // namespace app

#endif
