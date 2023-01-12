// Aseprite
// Copyright (C) 2021-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_BUTTON_H_INCLUDED
#define APP_UI_COLOR_BUTTON_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/context_observer.h"
#include "app/ui/color_button_options.h"
#include "app/ui/color_source.h"
#include "doc/pixel_format.h"
#include "obs/signal.h"
#include "ui/button.h"

namespace ui {
  class CloseEvent;
  class InitThemeEvent;
}

namespace app {
  class ColorPopup;

  class ColorButton : public ui::ButtonBase,
                      public ContextObserver,
                      public IColorSource {
  public:
    ColorButton(const app::Color& color,
                const PixelFormat pixelFormat,
                const ColorButtonOptions& options);
    ~ColorButton();

    PixelFormat pixelFormat() const;
    void setPixelFormat(PixelFormat pixelFormat);

    app::Color getColor() const;
    void setColor(const app::Color& color);

    bool isPopupVisible();
    void openPopup(const bool forcePinned);
    void closePopup();

    // IColorSource
    app::Color getColorByPosition(const gfx::Point& pos) override;

    // Signals
    obs::signal<void(app::Color&)> BeforeChange;
    obs::signal<void(const app::Color&)> Change;

  protected:
    // Events
    void onInitTheme(ui::InitThemeEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onClick() override;
    void onStartDrag() override;
    void onSelectWhenDragging() override;
    void onLoadLayout(ui::LoadLayoutEvent& ev) override;
    void onSaveLayout(ui::SaveLayoutEvent& ev) override;

  private:
    // ContextObserver impl
    void onActiveSiteChange(const Site& site) override;

    void onWindowClose(ui::CloseEvent& ev);
    void onWindowColorChange(const app::Color& color);
    bool canPin() const { return m_options.canPinSelector; }

    // Used to convert saved bounds (m_window/hiddenDefaultBounds,
    // which can be relative to the display or relative to the screen)
    // to the current system of coordinates.
    gfx::Rect convertBounds(const gfx::Rect& bounds) const;

    app::Color m_color;
    app::Color m_startDragColor;
    PixelFormat m_pixelFormat;
    ColorPopup* m_window;
    gfx::Rect m_windowDefaultBounds;
    gfx::Rect m_hiddenPopupBounds;
    bool m_desktopCoords;       // True if m_windowDefault/hiddenPopupBounds are screen coordinates
    bool m_dependOnLayer;
    bool m_mouseLeft;
    ColorButtonOptions m_options;
  };

} // namespace app

#endif
