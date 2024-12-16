// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_TINT_SHADE_TONE_H_INCLUDED
#define APP_UI_COLOR_TINT_SHADE_TONE_H_INCLUDED
#pragma once

#include "app/ui/color_selector.h"
#include "obs/connection.h"

namespace app {
class Color;

class ColorTintShadeTone : public ColorSelector {
public:
  ColorTintShadeTone();

protected:
#if SK_ENABLE_SKSL
  const char* getMainAreaShader() override;
  const char* getBottomBarShader() override;
  void setShaderParams(SkRuntimeShaderBuilder& builder, bool main) override;
#endif
  app::Color getMainAreaColor(const int u, const int umax, const int v, const int vmax) override;
  app::Color getBottomBarColor(const int u, const int umax) override;

  void onPaint(ui::PaintEvent& ev) override;
  void onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc) override;
  void onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc) override;
  void onPaintSurfaceInBgThread(os::Surface* s,
                                const gfx::Rect& main,
                                const gfx::Rect& bottom,
                                const gfx::Rect& alpha,
                                bool& stop) override;
  int onNeedsSurfaceRepaint(const app::Color& newColor) override;

private:
#if SK_ENABLE_SKSL
  std::string m_mainShader;
  std::string m_bottomShader;
#endif
  bool m_hueWithSatValue = false;
  obs::scoped_connection m_conn;
};

} // namespace app

#endif
