// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_COLOR_BUTTON_H_INCLUDED
#define APP_UI_COLOR_BUTTON_H_INCLUDED
#pragma once

#include "app/color.h"
#include "base/signal.h"
#include "doc/context_observer.h"
#include "doc/pixel_format.h"
#include "ui/button.h"

namespace app {
  class ColorPopup;

  class ColorButton : public ui::ButtonBase
                    , public doc::ContextObserver {
  public:
    ColorButton(const app::Color& color, PixelFormat pixelFormat);
    ~ColorButton();

    PixelFormat pixelFormat() const;
    void setPixelFormat(PixelFormat pixelFormat);

    app::Color getColor() const;
    void setColor(const app::Color& color);

    // Signals
    base::Signal1<void, const app::Color&> Change;

  protected:
    // Events
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onClick(ui::Event& ev) override;

  private:
    void openSelectorDialog();
    void closeSelectorDialog();
    void onWindowColorChange(const app::Color& color);
    void onActiveSiteChange(const Site& site) override;

    app::Color m_color;
    PixelFormat m_pixelFormat;
    ColorPopup* m_window;
    bool m_dependOnLayer;
  };

} // namespace app

#endif
