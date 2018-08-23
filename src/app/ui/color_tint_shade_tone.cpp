// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_tint_shade_tone.h"

#include "app/color_utils.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/graphics.h"

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

ColorTintShadeTone::ColorTintShadeTone()
{
}

app::Color ColorTintShadeTone::getMainAreaColor(const int u, const int umax,
                                                const int v, const int vmax)
{
  double sat = (1.0 * u / umax);
  double val = (1.0 - double(v) / double(vmax));
  return app::Color::fromHsv(
    m_color.getHsvHue(),
    MID(0.0, sat, 1.0),
    MID(0.0, val, 1.0),
    m_color.getAlpha());
}

app::Color ColorTintShadeTone::getBottomBarColor(const int u, const int umax)
{
  double hue = (360.0 * u / umax);
  return app::Color::fromHsv(
    MID(0.0, hue, 360.0),
    m_color.getHsvSaturation(),
    m_color.getHsvValue(),
    m_color.getAlpha());
}

void ColorTintShadeTone::onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc)
{
  if (m_color.getType() != app::Color::MaskType) {
    double sat = m_color.getHsvSaturation();
    double val = m_color.getHsvValue();
    gfx::Point pos(rc.x + int(sat * rc.w),
                   rc.y + int((1.0-val) * rc.h));

    paintColorIndicator(g, pos, val < 0.5);
  }
}

void ColorTintShadeTone::onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc)
{
  if (m_color.getType() != app::Color::MaskType) {
    double hue = m_color.getHsvHue();
    gfx::Point pos(rc.x + int(rc.w * hue / 360.0),
                   rc.y + rc.h/2);
    paintColorIndicator(g, pos, false);
  }
}

void ColorTintShadeTone::onPaintSurfaceInBgThread(
  os::Surface* s,
  const gfx::Rect& main,
  const gfx::Rect& bottom,
  const gfx::Rect& alpha,
  bool& stop)
{
  double hue = m_color.getHsvHue();
  int umax = MAX(1, main.w-1);
  int vmax = MAX(1, main.h-1);

  if (m_paintFlags & MainAreaFlag) {
    for (int y=0; y<main.h && !stop; ++y) {
      for (int x=0; x<main.w && !stop; ++x) {
        double sat = double(x) / double(umax);
        double val = 1.0 - double(y) / double(vmax);

        gfx::Color color = color_utils::color_for_ui(
          app::Color::fromHsv(
            hue,
            MID(0.0, sat, 1.0),
            MID(0.0, val, 1.0)));

        s->putPixel(color, main.x+x, main.y+y);
      }
    }
    if (stop)
      return;
    m_paintFlags ^= MainAreaFlag;
  }

  if (m_paintFlags & BottomBarFlag) {
    for (int x=0; x<bottom.w && !stop; ++x) {
      gfx::Color color = color_utils::color_for_ui(
        app::Color::fromHsv(
          (360.0 * x / bottom.w), 1.0, 1.0));

      s->drawVLine(color, bottom.x+x, bottom.y, bottom.h);
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
  return
    // Only if the hue changes we have to redraw the main surface.
    (cs_double_diff(m_color.getHsvHue(), newColor.getHsvHue()) ? MainAreaFlag: 0) |
    ColorSelector::onNeedsSurfaceRepaint(newColor);
}

} // namespace app
