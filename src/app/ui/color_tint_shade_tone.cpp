// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/color_tint_shade_tone.h"

#include "app/color_utils.h"
#include "app/pref/preferences.h"
#include "app/ui/skin/skin_theme.h"
#include "app/util/shader_helpers.h"
#include "ui/graphics.h"

#include <algorithm>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

ColorTintShadeTone::ColorTintShadeTone()
{
  m_conn = Preferences::instance().experimental.hueWithSatValueForColorSelector.AfterChange.connect(
    [this]() {
      m_paintFlags |= AllAreasFlag;

#if SK_ENABLE_SKSL
      m_bottomShader.clear();
      resetBottomEffect();
#endif

      invalidate();
    });
}

#if SK_ENABLE_SKSL

const char* ColorTintShadeTone::getMainAreaShader()
{
  if (m_mainShader.empty()) {
    m_mainShader += "uniform half3 iRes;"
                    "uniform half4 iHsv;";
    m_mainShader += kHSV_to_RGB_sksl;
    m_mainShader += R"(
half4 main(vec2 fragcoord) {
 vec2 d = fragcoord.xy / iRes.xy;
 half hue = iHsv.x;
 half sat = d.x;
 half val = 1.0 - d.y;
 return hsv_to_rgb(vec3(hue, sat, val)).rgb1;
}
)";
  }
  return m_mainShader.c_str();
}

const char* ColorTintShadeTone::getBottomBarShader()
{
  if (m_bottomShader.empty()) {
    m_bottomShader += "uniform half3 iRes;"
                      "uniform half4 iHsv;";
    m_bottomShader += kHSV_to_RGB_sksl;

    if (m_hueWithSatValue)
      m_bottomShader += R"(
half4 main(vec2 fragcoord) {
 half h = (fragcoord.x / iRes.x);
 return hsv_to_rgb(half3(h, iHsv.y, iHsv.z)).rgb1;
}
)";
    else
      m_bottomShader += R"(
half4 main(vec2 fragcoord) {
 half h = (fragcoord.x / iRes.x);
 return hsv_to_rgb(half3(h, 1.0, 1.0)).rgb1;
}
)";
  }
  return m_bottomShader.c_str();
}

void ColorTintShadeTone::setShaderParams(SkRuntimeShaderBuilder& builder, bool main)
{
  builder.uniform("iHsv") = appColorHsv_to_SkV4(m_color);
}

#endif // SK_ENABLE_SKSL

app::Color ColorTintShadeTone::getMainAreaColor(const int u,
                                                const int umax,
                                                const int v,
                                                const int vmax)
{
  double sat = (1.0 * u / umax);
  double val = (1.0 - double(v) / double(vmax));
  return app::Color::fromHsv(m_color.getHsvHue(),
                             std::clamp(sat, 0.0, 1.0),
                             std::clamp(val, 0.0, 1.0),
                             getCurrentAlphaForNewColor());
}

app::Color ColorTintShadeTone::getBottomBarColor(const int u, const int umax)
{
  double hue = (360.0 * u / umax);
  return app::Color::fromHsv(std::clamp(hue, 0.0, 360.0),
                             m_color.getHsvSaturation(),
                             m_color.getHsvValue(),
                             getCurrentAlphaForNewColor());
}

void ColorTintShadeTone::onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc)
{
  if (m_color.getType() != app::Color::MaskType) {
    double sat = m_color.getHsvSaturation();
    double val = m_color.getHsvValue();
    gfx::Point pos(rc.x + int(sat * rc.w), rc.y + int((1.0 - val) * rc.h));

    paintColorIndicator(g, pos, val < 0.5);
  }
}

void ColorTintShadeTone::onPaint(ui::PaintEvent& ev)
{
  m_hueWithSatValue = Preferences::instance().experimental.hueWithSatValueForColorSelector();
  ColorSelector::onPaint(ev);
}

void ColorTintShadeTone::onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc)
{
  if (m_color.getType() != app::Color::MaskType) {
    double hue = m_color.getHsvHue();
    double val;
    if (m_hueWithSatValue)
      val = m_color.getHsvValue();
    else
      val = 1.0;

    gfx::Point pos(rc.x + int(rc.w * hue / 360.0), rc.y + rc.h / 2);
    paintColorIndicator(g, pos, val < 0.5);
  }
}

void ColorTintShadeTone::onPaintSurfaceInBgThread(os::Surface* s,
                                                  const gfx::Rect& main,
                                                  const gfx::Rect& bottom,
                                                  const gfx::Rect& alpha,
                                                  bool& stop)
{
  double hue = m_color.getHsvHue();
  int umax = std::max(1, main.w - 1);
  int vmax = std::max(1, main.h - 1);

  if (m_paintFlags & MainAreaFlag) {
    for (int y = 0; y < main.h && !stop; ++y) {
      for (int x = 0; x < main.w && !stop; ++x) {
        double sat = double(x) / double(umax);
        double val = 1.0 - double(y) / double(vmax);

        gfx::Color color = color_utils::color_for_ui(
          app::Color::fromHsv(hue, std::clamp(sat, 0.0, 1.0), std::clamp(val, 0.0, 1.0)));

        s->putPixel(color, main.x + x, main.y + y);
      }
    }
    if (stop)
      return;
    m_paintFlags ^= MainAreaFlag;
  }

  if (m_paintFlags & BottomBarFlag) {
    os::Paint paint;
    double sat, val;

    if (m_hueWithSatValue) {
      sat = m_color.getHsvSaturation();
      val = m_color.getHsvValue();
    }
    else {
      sat = 1.0;
      val = 1.0;
    }

    for (int x = 0; x < bottom.w && !stop; ++x) {
      paint.color(color_utils::color_for_ui(app::Color::fromHsv((360.0 * x / bottom.w), sat, val)));

      s->drawRect(gfx::Rect(bottom.x + x, bottom.y, 1, bottom.h), paint);
    }
    if (stop)
      return;
    m_paintFlags ^= BottomBarFlag;
  }

  // Paint alpha bar
  ColorSelector::onPaintSurfaceInBgThread(s, main, bottom, alpha, stop);
}

int ColorTintShadeTone::onNeedsSurfaceRepaint(const app::Color& newColor)
{
  int flags =
    // Only if the hue changes we have to redraw the main surface.
    (cs_double_diff(m_color.getHsvHue(), newColor.getHsvHue()) ? MainAreaFlag : 0) |
    ColorSelector::onNeedsSurfaceRepaint(newColor);

  if (m_hueWithSatValue) {
    flags |= (cs_double_diff(m_color.getHsvSaturation(), newColor.getHsvSaturation()) ||
                  cs_double_diff(m_color.getHsvValue(), newColor.getHsvValue()) ?
                BottomBarFlag :
                0);
  }
  return flags;
}

} // namespace app
