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

ColorTintShadeTone::ColorTintShadeTone()
  : m_capturedInHue(false)
{
  setBorder(gfx::Border(3*ui::guiscale()));
}

app::Color ColorTintShadeTone::getColorByPosition(const gfx::Point& pos)
{
  gfx::Rect rc = childrenBounds();
  if (rc.isEmpty())
    return app::Color::fromMask();

  int u, v, umax, vmax;
  int huebar = getHueBarSize();
  u = pos.x - rc.x;
  v = pos.y - rc.y;
  umax = MAX(1, rc.w-1);
  vmax = MAX(1, rc.h-1-huebar);

  double hue, sat, val;

  bool inHue =
    (( hasCapture() && m_capturedInHue) ||
     (!hasCapture() && inHueBarArea(pos)));

  if (inHue) {
    hue = (360.0 * u / umax);
    sat = m_color.getHsvSaturation();
    val = m_color.getHsvValue();
  }
  else {
    hue = m_color.getHsvHue();
    sat = (1.0 * u / umax);
    val = (1.0 - double(v) / double(vmax));
  }

  return app::Color::fromHsv(
    MID(0.0, hue, 360.0),
    MID(0.0, sat, 1.0),
    MID(0.0, val, 1.0));
}

void ColorTintShadeTone::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  theme->drawRect(g, clientBounds(),
                  theme->parts.editorNormal().get(),
                  false);       // Do not fill the center

  gfx::Rect rc = clientChildrenBounds();
  if (rc.isEmpty())
    return;

  double hue = m_color.getHsvHue();
  int umax, vmax;
  int huebar = getHueBarSize();
  umax = MAX(1, rc.w-1);
  vmax = MAX(1, rc.h-1-huebar);

  for (int y=0; y<rc.h-huebar; ++y) {
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

  if (huebar > 0) {
    for (int y=rc.h-huebar; y<rc.h; ++y) {
      for (int x=0; x<rc.w; ++x) {
        gfx::Color color = color_utils::color_for_ui(
          app::Color::fromHsv(
            (360.0 * x / rc.w), 1.0, 1.0));

        g->putPixel(color, rc.x+x, rc.y+y);
      }
    }
  }

  if (m_color.getType() != app::Color::MaskType) {
    double sat = m_color.getHsvSaturation();
    double val = m_color.getHsvValue();
    gfx::Point pos(rc.x + int(sat * rc.w),
                   rc.y + int((1.0-val) * (rc.h-huebar)));

    she::Surface* icon = theme->parts.colorWheelIndicator()->bitmap(0);
    g->drawColoredRgbaSurface(
      icon,
      val > 0.5 ? gfx::rgba(0, 0, 0): gfx::rgba(255, 255, 255),
      pos.x-icon->width()/2,
      pos.y-icon->height()/2);

    if (huebar > 0) {
      pos.x = rc.x + int(rc.w * hue / 360.0);
      pos.y = rc.y + rc.h - huebar/2;
      g->drawColoredRgbaSurface(
        icon,
        gfx::rgba(0, 0, 0),
        pos.x-icon->width()/2,
        pos.y-icon->height()/2);
    }
  }
}

bool ColorTintShadeTone::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage:
      if (manager()->getCapture())
        break;

      captureMouse();

      // Continue...

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

      if (msg->type() == kMouseDownMessage)
          m_capturedInHue = inHueBarArea(mouseMsg->position());

      app::Color color = getColorByPosition(mouseMsg->position());
      if (color != app::Color::fromMask()) {
        StatusBar::instance()->showColor(0, "", color);
        if (hasCapture())
          ColorChange(color, mouseMsg->buttons());
      }
      break;
    }

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();
      }
      return true;

    case kSetCursorMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      if (childrenBounds().contains(mouseMsg->position())) {
        ui::set_mouse_cursor(kCustomCursor, SkinTheme::instance()->cursors.eyedropper());
        return true;
      }
      break;
    }

  }

  return ColorSelector::onProcessMessage(msg);
}

bool ColorTintShadeTone::inHueBarArea(const gfx::Point& pos) const
{
  gfx::Rect rc = childrenBounds();
  if (rc.isEmpty() || !rc.contains(pos))
    return false;
  else
    return (pos.y >= rc.y+rc.h-getHueBarSize());
}

int ColorTintShadeTone::getHueBarSize() const
{
  gfx::Rect rc = clientChildrenBounds();
  int size = 8*guiscale();
  return rc.h < 2*size ? 0: size;
}

} // namespace app
