// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_BUTTON_H_INCLUDED
#define APP_UI_COLOR_BUTTON_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/ui/color_source.h"
#include "doc/context_observer.h"
#include "doc/pixel_format.h"
#include "obs/signal.h"
#include "ui/button.h"

namespace ui {
  class InitThemeEvent;
}

namespace app {
  class ColorPopup;

  class ColorButton : public ui::ButtonBase
                    , public doc::ContextObserver
                    , public IColorSource {
  public:
    ColorButton(const app::Color& color,
                const PixelFormat pixelFormat,
                const bool canPinSelector,
                const bool showSimpleColors);
    ~ColorButton();

    PixelFormat pixelFormat() const;
    void setPixelFormat(PixelFormat pixelFormat);

    app::Color getColor() const;
    void setColor(const app::Color& color);

    // IColorSource
    app::Color getColorByPosition(const gfx::Point& pos) override;

    // Signals
    obs::signal<void(const app::Color&)> Change;

  protected:
    // Events
    void onInitTheme(ui::InitThemeEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onClick(ui::Event& ev) override;
    void onLoadLayout(ui::LoadLayoutEvent& ev) override;
    void onSaveLayout(ui::SaveLayoutEvent& ev) override;

  private:
    void openSelectorDialog();
    void closeSelectorDialog();
    void onWindowColorChange(const app::Color& color);
    void onActiveSiteChange(const Site& site) override;

    app::Color m_color;
    PixelFormat m_pixelFormat;
    ColorPopup* m_window;
    gfx::Rect m_windowDefaultBounds;
    bool m_dependOnLayer;
    bool m_canPinSelector;
    bool m_showSimpleColors;
  };

} // namespace app

#endif
