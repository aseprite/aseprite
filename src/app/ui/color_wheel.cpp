// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_wheel.h"

#include "app/color_utils.h"
#include "app/pref/preferences.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "base/bind.h"
#include "base/pi.h"
#include "base/scoped_value.h"
#include "filters/color_curve.h"
#include "she/surface.h"
#include "ui/graphics.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

static struct {
  int n;
  int hues[4];
  int sats[4];
} harmonies[] = {
  { 1, { 0,   0,   0,   0 }, { 100,   0,   0,   0 } }, // NONE
  { 2, { 0, 180,   0,   0 }, { 100, 100,   0,   0 } }, // COMPLEMENTARY
  { 2, { 0,   0,   0,   0 }, { 100,  50,   0,   0 } }, // MONOCHROMATIC
  { 3, { 0,  30, 330,   0 }, { 100, 100, 100,   0 } }, // ANALOGOUS
  { 3, { 0, 150, 210,   0 }, { 100, 100, 100,   0 } }, // SPLIT
  { 3, { 0, 120, 240,   0 }, { 100, 100, 100,   0 } }, // TRIADIC
  { 4, { 0, 120, 180, 300 }, { 100, 100, 100, 100 } }, // TETRADIC
  { 4, { 0,  90, 180, 270 }, { 100, 100, 100, 100 } }, // SQUARE
};

ColorWheel::ColorWheel()
  : m_discrete(Preferences::instance().colorBar.discreteWheel())
  , m_colorModel((ColorModel)Preferences::instance().colorBar.wheelModel())
  , m_harmony((Harmony)Preferences::instance().colorBar.harmony())
  , m_options("")
  , m_harmonyPicked(false)
{
  SkinTheme* theme = SkinTheme::instance();

  m_options.Click.connect(base::Bind<void>(&ColorWheel::onOptions, this));
  m_options.setStyle(theme->styles.colorWheelOptions());

  addChild(&m_options);
}

app::Color ColorWheel::getMainAreaColor(const int _u, const int umax,
                                        const int _v, const int vmax)
{
  m_harmonyPicked = false;

  int u = _u - umax/2;
  int v = _v - vmax/2;
  double d = std::sqrt(u*u + v*v);

  // Pick from the wheel
  if (d < m_wheelRadius+2*guiscale()) {
    double a = std::atan2(-v, u);

    int hue = (int(180.0 * a / PI)
               + 180            // To avoid [-180,0) range
               + 180 + 30       // To locate green at 12 o'clock
               );
    if (m_discrete) {
      hue += 15;
      hue /= 30;
      hue *= 30;
    }
    hue %= 360;                 // To leave hue in [0,360) range
    hue = convertHueAngle(hue, 1);

    int sat;
    if (m_discrete) {
      sat = int(120.0 * d / m_wheelRadius);
      sat /= 20;
      sat *= 20;
    }
    else {
      sat = int(100.0 * d / m_wheelRadius);
    }

    return app::Color::fromHsv(
      MID(0, hue, 360),
      MID(0, sat / 100.0, 1.0),
      m_color.getHsvValue());
  }

  // Pick harmonies
  if (m_color.getAlpha() > 0) {
    const gfx::Rect& rc = m_clientBounds;
    int n = getHarmonies();
    int boxsize = MIN(rc.w/10, rc.h/10);

    for (int i=0; i<n; ++i) {
      app::Color color = getColorInHarmony(i);

      if (gfx::Rect(rc.x+rc.w-(n-i)*boxsize,
                    rc.y+rc.h-boxsize,
                    boxsize, boxsize).contains(gfx::Point(u, v))) {
        m_harmonyPicked = true;

        color = app::Color::fromHsv(convertHueAngle(int(color.getHsvHue()), 1),
                                    color.getHsvSaturation(),
                                    color.getHsvValue());
        return color;
      }
    }
  }

  return app::Color::fromMask();
}

app::Color ColorWheel::getBottomBarColor(const int u, const int umax)
{
  double val = double(u) / double(umax);
  return app::Color::fromHsv(
    m_color.getHsvHue(),
    m_color.getHsvSaturation(),
    MID(0.0, val, 1.0));
}

void ColorWheel::onPaintMainArea(ui::Graphics* g, const gfx::Rect& rc)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  int r = MIN(rc.w/2, rc.h/2);
  m_clientBounds = rc;
  m_wheelRadius = r;
  m_wheelBounds = gfx::Rect(rc.x+rc.w/2-r,
                            rc.y+rc.h/2-r,
                            r*2, r*2);

  int umax = MAX(1, rc.w-1);
  int vmax = MAX(1, rc.h-1);

  for (int y=0; y<rc.h; ++y) {
    for (int x=0; x<rc.w; ++x) {
      app::Color appColor =
        getMainAreaColor(x, umax,
                         y, vmax);

      gfx::Color color;
      if (appColor.getType() != app::Color::MaskType) {
        color = color_utils::color_for_ui(appColor);
      }
      else {
        color = theme->colors.editorFace();
      }

      g->putPixel(color, rc.x+x, rc.y+y);
    }
  }

  if (m_color.getAlpha() > 0) {
    int n = getHarmonies();
    int boxsize = MIN(rc.w/10, rc.h/10);

    for (int i=0; i<n; ++i) {
      app::Color color = getColorInHarmony(i);
      double angle = color.getHsvHue()-30.0;
      double dist = color.getHsvSaturation();

      color = app::Color::fromHsv(convertHueAngle(int(color.getHsvHue()), 1),
                                  color.getHsvSaturation(),
                                  color.getHsvValue());

      gfx::Point pos =
        m_wheelBounds.center() +
        gfx::Point(int(+std::cos(PI*angle/180.0)*double(m_wheelRadius)*dist),
                   int(-std::sin(PI*angle/180.0)*double(m_wheelRadius)*dist));

      paintColorIndicator(g, pos, false);

      g->fillRect(gfx::rgba(color.getRed(),
                            color.getGreen(),
                            color.getBlue(), 255),
                  gfx::Rect(rc.x+rc.w-(n-i)*boxsize,
                            rc.y+rc.h-boxsize,
                            boxsize, boxsize));
    }
  }
}

void ColorWheel::onPaintBottomBar(ui::Graphics* g, const gfx::Rect& rc)
{
  double hue = m_color.getHsvHue();
  double sat = m_color.getHsvValue();

  for (int x=0; x<rc.w; ++x) {
    gfx::Color color = color_utils::color_for_ui(
      app::Color::fromHsv(hue, sat, double(x) / double(rc.w)));

    g->drawVLine(color, rc.x+x, rc.y, rc.h);
  }

  if (m_color.getType() != app::Color::MaskType) {
    double val = m_color.getHsvValue();
    gfx::Point pos(rc.x + int(double(rc.w) * val),
                   rc.y + rc.h/2);
    paintColorIndicator(g, pos, false);
  }
}

void ColorWheel::setDiscrete(bool state)
{
  m_discrete = state;
  Preferences::instance().colorBar.discreteWheel(m_discrete);

  invalidate();
}

void ColorWheel::setColorModel(ColorModel colorModel)
{
  m_colorModel = colorModel;
  Preferences::instance().colorBar.wheelModel((int)m_colorModel);

  invalidate();
}

void ColorWheel::setHarmony(Harmony harmony)
{
  m_harmony = harmony;
  Preferences::instance().colorBar.harmony((int)m_harmony);

  invalidate();
}

int ColorWheel::getHarmonies() const
{
  int i = MID(0, (int)m_harmony, (int)Harmony::LAST);
  return harmonies[i].n;
}

app::Color ColorWheel::getColorInHarmony(int j) const
{
  int i = MID(0, (int)m_harmony, (int)Harmony::LAST);
  j = MID(0, j, harmonies[i].n-1);
  double hue = convertHueAngle(int(m_color.getHsvHue()), -1) + harmonies[i].hues[j];
  double sat = m_color.getHsvSaturation() * harmonies[i].sats[j] / 100.0;
  return app::Color::fromHsv(std::fmod(hue, 360),
                             MID(0.0, sat, 1.0),
                             m_color.getHsvValue());
}

void ColorWheel::onResize(ui::ResizeEvent& ev)
{
  ColorSelector::onResize(ev);

  gfx::Rect rc = clientChildrenBounds();
  gfx::Size prefSize = m_options.sizeHint();
  rc = childrenBounds();
  rc.x += rc.w-prefSize.w;
  rc.w = prefSize.w;
  rc.h = prefSize.h;
  m_options.setBounds(rc);
}

void ColorWheel::onOptions()
{
  Menu menu;
  MenuItem discrete("Discrete");
  MenuItem none("Without Harmonies");
  MenuItem complementary("Complementary");
  MenuItem monochromatic("Monochromatic");
  MenuItem analogous("Analogous");
  MenuItem split("Split-Complementary");
  MenuItem triadic("Triadic");
  MenuItem tetradic("Tetradic");
  MenuItem square("Square");
  menu.addChild(&discrete);
  menu.addChild(new MenuSeparator);
  menu.addChild(&none);
  menu.addChild(&complementary);
  menu.addChild(&monochromatic);
  menu.addChild(&analogous);
  menu.addChild(&split);
  menu.addChild(&triadic);
  menu.addChild(&tetradic);
  menu.addChild(&square);

  if (isDiscrete()) discrete.setSelected(true);
  switch (m_harmony) {
    case Harmony::NONE: none.setSelected(true); break;
    case Harmony::COMPLEMENTARY: complementary.setSelected(true); break;
    case Harmony::MONOCHROMATIC: monochromatic.setSelected(true); break;
    case Harmony::ANALOGOUS: analogous.setSelected(true); break;
    case Harmony::SPLIT: split.setSelected(true); break;
    case Harmony::TRIADIC: triadic.setSelected(true); break;
    case Harmony::TETRADIC: tetradic.setSelected(true); break;
    case Harmony::SQUARE: square.setSelected(true); break;
  }

  discrete.Click.connect(base::Bind<void>(&ColorWheel::setDiscrete, this, !isDiscrete()));
  none.Click.connect(base::Bind<void>(&ColorWheel::setHarmony, this, Harmony::NONE));
  complementary.Click.connect(base::Bind<void>(&ColorWheel::setHarmony, this, Harmony::COMPLEMENTARY));
  monochromatic.Click.connect(base::Bind<void>(&ColorWheel::setHarmony, this, Harmony::MONOCHROMATIC));
  analogous.Click.connect(base::Bind<void>(&ColorWheel::setHarmony, this, Harmony::ANALOGOUS));
  split.Click.connect(base::Bind<void>(&ColorWheel::setHarmony, this, Harmony::SPLIT));
  triadic.Click.connect(base::Bind<void>(&ColorWheel::setHarmony, this, Harmony::TRIADIC));
  tetradic.Click.connect(base::Bind<void>(&ColorWheel::setHarmony, this, Harmony::TETRADIC));
  square.Click.connect(base::Bind<void>(&ColorWheel::setHarmony, this, Harmony::SQUARE));

  gfx::Rect rc = m_options.bounds();
  menu.showPopup(gfx::Point(rc.x+rc.w, rc.y));
}

int ColorWheel::convertHueAngle(int hue, int dir) const
{
  switch (m_colorModel) {

    case ColorModel::RGB:
      return hue;

    case ColorModel::RYB: {
      static std::vector<int> map1;
      static std::vector<int> map2;

      if (map2.empty()) {
        filters::ColorCurve curve1(filters::ColorCurve::Linear);
        curve1.addPoint(gfx::Point(0, 0));
        curve1.addPoint(gfx::Point(60, 35));
        curve1.addPoint(gfx::Point(122, 60));
        curve1.addPoint(gfx::Point(165, 120));
        curve1.addPoint(gfx::Point(218, 180));
        curve1.addPoint(gfx::Point(275, 240));
        curve1.addPoint(gfx::Point(330, 300));
        curve1.addPoint(gfx::Point(360, 360));

        filters::ColorCurve curve2(filters::ColorCurve::Linear);
        for (const auto& pt : curve1)
          curve2.addPoint(gfx::Point(pt.y, pt.x));

        map1.resize(360);
        map2.resize(360);
        curve1.getValues(0, 359, map1);
        curve2.getValues(0, 359, map2);
      }

      if (hue < 0)
        hue += 360;
      hue %= 360;

      ASSERT(hue >= 0 && hue < 360);
      if (dir > 0)
        return map1[hue];
      else if (dir < 0)
        return map2[hue];
    }

  }
  return hue;
}

} // namespace app
