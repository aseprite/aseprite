// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_SELECTOR_H_INCLUDED
#define APP_UI_COLOR_SELECTOR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/ui/color_source.h"
#include "obs/connection.h"
#include "obs/signal.h"
#include "os/surface.h"
#include "ui/mouse_button.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <atomic>
#include <cmath>

// TODO We should wrap the SkRuntimeEffect in laf-os, SkRuntimeEffect
//      and SkRuntimeShaderBuilder might change in future Skia
//      versions.
#if SK_ENABLE_SKSL
  #include "include/effects/SkRuntimeEffect.h"
#endif

// TODO move this to laf::base
inline bool cs_double_diff(double a, double b) {
  return std::fabs((a)-(b)) > 0.001;
}

namespace app {

  class ColorSelector : public ui::Widget
                      , public IColorSource {
  public:
    class Painter;

    ColorSelector();
    ~ColorSelector();

    void selectColor(const app::Color& color);

    // IColorSource impl
    app::Color getColorByPosition(const gfx::Point& pos) override;

    // Signals
    obs::signal<void(const app::Color&, ui::MouseButton)> ColorChange;

  protected:
    // paintFlags for onPaintSurfaceInBgThread and return value of
    // onNeedsSurfaceRepaint().
    enum {
      MainAreaFlag  = 1,
      BottomBarFlag = 2,
      AlphaBarFlag  = 4,
      AllAreasFlag  = MainAreaFlag | BottomBarFlag | AlphaBarFlag,
      DoneFlag      = 8,
    };

    void onSizeHint(ui::SizeHintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    void onInitTheme(ui::InitThemeEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

    virtual const char* getMainAreaShader() { return nullptr; }
    virtual const char* getBottomBarShader() { return nullptr; }
#if SK_ENABLE_SKSL
    virtual void setShaderParams(SkRuntimeShaderBuilder& builder, bool main) { }
#endif
    virtual app::Color getMainAreaColor(const int u, const int umax,
                                        const int v, const int vmax) = 0;
    virtual app::Color getBottomBarColor(const int u, const int umax) = 0;
    virtual void onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc) = 0;
    virtual void onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc) = 0;
    virtual void onPaintSurfaceInBgThread(os::Surface* s,
                                          const gfx::Rect& main,
                                          const gfx::Rect& bottom,
                                          const gfx::Rect& alpha,
                                          bool& stop);
    virtual int onNeedsSurfaceRepaint(const app::Color& newColor);
    virtual bool subColorPicked() { return false; }

    void paintColorIndicator(ui::Graphics* g,
                             const gfx::Point& pos,
                             bool white,
                             int alpha=255);
    void paintColorIndicatorChain(ui::Graphics* g,
        const std::vector<gfx::Point>& positions,
        const std::vector<bool>& white,
        int current);

    // Returns the 255 if m_color is the mask color, or the
    // m_color.getAlpha() if it's really a color.
    int getCurrentAlphaForNewColor() const;

    bool hasCaptureInMainArea() const { return m_capturedInMain; }

    app::Color m_color;

    // These flags indicate which areas must be redrawed in the
    // background thread. Equal to DoneFlag when the surface is
    // already painted in the background thread surface. This must be
    // atomic because we need atomic bitwise operations.
    std::atomic<int> m_paintFlags;

  protected:
#if SK_ENABLE_SKSL
    void resetBottomEffect();
#endif

  private:
    app::Color getAlphaBarColor(const int u, const int umax);
    void onPaintAlphaBar(ui::Graphics* g, const gfx::Rect& rc);

    gfx::Rect bottomBarBounds() const;
    gfx::Rect alphaBarBounds() const;

    void updateColorSpace();

#if SK_ENABLE_SKSL
    static const char* getAlphaBarShader();
    bool buildEffects();
#endif

    // Internal flag used to lock the modification of m_color.
    // E.g. When the user picks a color harmony, we don't want to
    // change the main color.
    bool m_lockColor;

    // True when the user pressed the mouse button in the bottom
    // slider. It's used to avoid swapping in both areas (main color
    // area vs bottom slider) when we drag the mouse above this
    // widget.
    bool m_capturedInBottom = false;
    bool m_capturedInAlpha = false;
    bool m_capturedInMain = false;

    ui::Timer m_timer;

    obs::scoped_connection m_appConn;

#if SK_ENABLE_SKSL
    // Shaders
    sk_sp<SkRuntimeEffect> m_mainEffect;
    sk_sp<SkRuntimeEffect> m_bottomEffect;
    static sk_sp<SkRuntimeEffect> m_alphaEffect;
#endif
  };

} // namespace app

#endif
