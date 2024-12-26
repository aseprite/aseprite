// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/color_spectrum.h"

#include "app/color_utils.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/util/shader_helpers.h"
#include "os/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"

#include <algorithm>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

ColorSpectrum::ColorSpectrum()
{
}

#if SK_ENABLE_SKSL

const char* ColorSpectrum::getMainAreaShader()
{
  if (m_mainShader.empty()) {
    m_mainShader += "uniform half3 iRes;"
                    "uniform half4 iHsl;";
    m_mainShader += kHSL_to_RGB_sksl;
    m_mainShader += R"(
half4 main(vec2 fragcoord) {
 vec2 d = fragcoord.xy / iRes.xy;
 half hue = d.x;
 half sat = iHsl.y;
 half lit = 1.0 - d.y;
 return hsl_to_rgb(half3(hue, sat, lit)).rgb1;
}
)";
  }
  return m_mainShader.c_str();
}

const char* ColorSpectrum::getBottomBarShader()
{
  if (m_bottomShader.empty()) {
    m_bottomShader += "uniform half3 iRes;"
                      "uniform half4 iHsl;";
    m_bottomShader += kHSL_to_RGB_sksl;
    m_bottomShader += R"(
half4 main(vec2 fragcoord) {
 half s = (fragcoord.x / iRes.x);
 return hsl_to_rgb(half3(iHsl.x, s, iHsl.z)).rgb1;
}
)";
  }
  return m_bottomShader.c_str();
}

void ColorSpectrum::setShaderParams(SkRuntimeShaderBuilder& builder, bool main)
{
  builder.uniform("iHsl") = appColorHsl_to_SkV4(m_color);
}

#endif // SK_ENABLE_SKSL

app::Color ColorSpectrum::getMainAreaColor(const int u, const int umax, const int v, const int vmax)
{
  double hue = 360.0 * u / umax;
  double lit = 1.0 - (double(v) / double(vmax));
  return app::Color::fromHsl(std::clamp(hue, 0.0, 360.0),
                             m_color.getHslSaturation(),
                             std::clamp(lit, 0.0, 1.0),
                             getCurrentAlphaForNewColor());
}

app::Color ColorSpectrum::getBottomBarColor(const int u, const int umax)
{
  double sat = double(u) / double(umax);
  return app::Color::fromHsl(m_color.getHslHue(),
                             std::clamp(sat, 0.0, 1.0),
                             m_color.getHslLightness(),
                             getCurrentAlphaForNewColor());
}

void ColorSpectrum::onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc)
{
  if (m_color.getType() != app::Color::MaskType) {
    double hue = m_color.getHslHue();
    double lit = m_color.getHslLightness();
    gfx::Point pos(rc.x + int(hue * rc.w / 360.0), rc.y + rc.h - int(lit * rc.h));

    paintColorIndicator(g, pos, lit < 0.5);
  }
}

void ColorSpectrum::onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc)
{
  double lit = m_color.getHslLightness();

  if (m_color.getType() != app::Color::MaskType) {
    double sat = m_color.getHslSaturation();
    gfx::Point pos(rc.x + int(double(rc.w) * sat), rc.y + rc.h / 2);
    paintColorIndicator(g, pos, lit < 0.5);
  }
}

void ColorSpectrum::onPaintSurfaceInBgThread(os::Surface* s,
                                             const gfx::Rect& main,
                                             const gfx::Rect& bottom,
                                             const gfx::Rect& alpha,
                                             bool& stop)
{
  if (m_paintFlags & MainAreaFlag) {
    double sat = m_color.getHslSaturation();
    int umax = std::max(1, main.w - 1);
    int vmax = std::max(1, main.h - 1);

    for (int y = 0; y < main.h && !stop; ++y) {
      for (int x = 0; x < main.w && !stop; ++x) {
        double hue = 360.0 * double(x) / double(umax);
        double lit = 1.0 - double(y) / double(vmax);

        gfx::Color color = color_utils::color_for_ui(
          app::Color::fromHsl(std::clamp(hue, 0.0, 360.0), sat, std::clamp(lit, 0.0, 1.0)));

        s->putPixel(color, main.x + x, main.y + y);
      }
    }
    if (stop)
      return;
    m_paintFlags ^= MainAreaFlag;
  }

  if (m_paintFlags & BottomBarFlag) {
    double lit = m_color.getHslLightness();
    double hue = m_color.getHslHue();
    os::Paint paint;
    for (int x = 0; x < bottom.w && !stop; ++x) {
      paint.color(
        color_utils::color_for_ui(app::Color::fromHsl(hue, double(x) / double(bottom.w), lit)));
      s->drawRect(gfx::Rect(bottom.x + x, bottom.y, 1, bottom.h), paint);
    }
    if (stop)
      return;
    m_paintFlags ^= BottomBarFlag;
  }

  // Paint alpha bar
  ColorSelector::onPaintSurfaceInBgThread(s, main, bottom, alpha, stop);
}

int ColorSpectrum::onNeedsSurfaceRepaint(const app::Color& newColor)
{
  return
    // Only if the saturation changes we have to redraw the main surface.
    (cs_double_diff(m_color.getHslSaturation(), newColor.getHslSaturation()) ? MainAreaFlag : 0) |
    (cs_double_diff(m_color.getHslHue(), newColor.getHslHue()) ||
         cs_double_diff(m_color.getHslLightness(), newColor.getHslLightness()) ?
       BottomBarFlag :
       0) |
    ColorSelector::onNeedsSurfaceRepaint(newColor);
}

} // namespace app
