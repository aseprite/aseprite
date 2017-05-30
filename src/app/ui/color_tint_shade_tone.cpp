// Aseprite
// Copyright (C) 2016-2017  David Capello
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
  double hue = m_color.getHsvHue();
  int umax = MAX(1, rc.w-1);
  int vmax = MAX(1, rc.h-1);

  for (int y=0; y<rc.h; ++y) {
    for (int x=0; x<rc.w; ++x) {
      double sat = double(x) / double(umax);
      double val = 1.0 - double(y) / double(vmax);

      gfx::Color color = color_utils::color_for_ui(
        app::Color::fromHsv(
          hue,
          MID(0.0, sat, 1.0),
          MID(0.0, val, 1.0)));

      g->putPixel(color, rc.x+x, rc.y+y);
    }
  }

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
  for (int x=0; x<rc.w; ++x) {
    gfx::Color color = color_utils::color_for_ui(
      app::Color::fromHsv(
        (360.0 * x / rc.w), 1.0, 1.0));

    g->drawVLine(color, rc.x+x, rc.y, rc.h);
  }

  if (m_color.getType() != app::Color::MaskType) {
    double hue = m_color.getHsvHue();
    gfx::Point pos(rc.x + int(rc.w * hue / 360.0),
                   rc.y + rc.h/2);
    paintColorIndicator(g, pos, false);
  }
}

} // namespace app
