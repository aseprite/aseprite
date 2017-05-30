// Aseprite
// Copyright (C) 2016-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_SELECTOR_H_INCLUDED
#define APP_UI_COLOR_SELECTOR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/ui/color_source.h"
#include "obs/signal.h"
#include "ui/mouse_buttons.h"
#include "ui/widget.h"

namespace app {

  class ColorSelector : public ui::Widget
                      , public IColorSource {
  public:
    ColorSelector();

    void selectColor(const app::Color& color);

    // IColorSource impl
    app::Color getColorByPosition(const gfx::Point& pos) override;

    // Signals
    obs::signal<void(const app::Color&, ui::MouseButtons)> ColorChange;

  protected:
    virtual app::Color getMainAreaColor(const int u, const int umax,
                                        const int v, const int vmax) = 0;
    virtual app::Color getBottomBarColor(const int u, const int umax) = 0;
    virtual void onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc) = 0;
    virtual void onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc) = 0;
    virtual bool subColorPicked() { return false; }

    void paintColorIndicator(ui::Graphics* g,
                             const gfx::Point& pos,
                             const bool white);

    app::Color m_color;

  private:
    void onSizeHint(ui::SizeHintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    void onPaint(ui::PaintEvent& ev) override;

    app::Color getAlphaBarColor(const int u, const int umax);
    void onPaintAlphaBar(ui::Graphics* g, const gfx::Rect& rc);

    gfx::Rect bottomBarBounds() const;
    gfx::Rect alphaBarBounds() const;

    // Internal flag used to lock the modification of m_color.
    // E.g. When the user picks a color harmony, we don't want to
    // change the main color.
    bool m_lockColor;

    // True when the user pressed the mouse button in the bottom
    // slider. It's used to avoid swapping in both areas (main color
    // area vs bottom slider) when we drag the mouse above this
    // widget.
    bool m_capturedInBottom;
    bool m_capturedInAlpha;
  };

} // namespace app

#endif
