// Aseprite
// Copyright (C) 2001-2017  David Capello
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
#include "she/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/resize_event.h"
#include "ui/system.h"

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

ColorSpectrum::ColorSpectrum()
{
}

app::Color ColorSpectrum::getMainAreaColor(const int u, const int umax,
                                           const int v, const int vmax)
{
  double hue = 360.0 * u / umax;
  double lit = 1.0 - (double(v)/double(vmax));
  return app::Color::fromHsl(
    MID(0.0, hue, 360.0),
    m_color.getHslSaturation(),
    MID(0.0, lit, 1.0),
    m_color.getAlpha());
}

app::Color ColorSpectrum::getBottomBarColor(const int u, const int umax)
{
  double sat = double(u) / double(umax);
  return app::Color::fromHsl(
    m_color.getHslHue(),
    MID(0.0, sat, 1.0),
    m_color.getHslLightness(),
    m_color.getAlpha());
}

void ColorSpectrum::onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc)
{
  double sat = m_color.getHslSaturation();
  int umax = MAX(1, rc.w-1);
  int vmax = MAX(1, rc.h-1);

  for (int y=0; y<rc.h; ++y) {
    for (int x=0; x<rc.w; ++x) {
      double hue = 360.0 * double(x) / double(umax);
      double lit = 1.0 - double(y) / double(vmax);

      gfx::Color color = color_utils::color_for_ui(
        app::Color::fromHsl(
          MID(0.0, hue, 360.0),
          sat,
          MID(0.0, lit, 1.0)));

      g->putPixel(color, rc.x+x, rc.y+y);
    }
  }

  if (m_color.getType() != app::Color::MaskType) {
    double hue = m_color.getHslHue();
    double lit = m_color.getHslLightness();
    gfx::Point pos(rc.x + int(hue * rc.w / 360.0),
                   rc.y + rc.h - int(lit * rc.h));

    paintColorIndicator(g, pos, lit < 0.5);
  }
}

void ColorSpectrum::onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc)
{
  double hue = m_color.getHslHue();
  double lit = m_color.getHslLightness();

  for (int x=0; x<rc.w; ++x) {
    gfx::Color color = color_utils::color_for_ui(
      app::Color::fromHsl(hue, double(x) / double(rc.w), lit));

    g->drawVLine(color, rc.x+x, rc.y, rc.h);
  }

  if (m_color.getType() != app::Color::MaskType) {
    double sat = m_color.getHslSaturation();
    gfx::Point pos(rc.x + int(double(rc.w) * sat),
                   rc.y + rc.h/2);
    paintColorIndicator(g, pos, false);
  }
}

} // namespace app
