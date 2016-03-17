// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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

app::Color ColorTintShadeTone::pickColor(const gfx::Point& pos) const
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

  if (m_capturedInHue) {
    hue = (360.0 * u / umax);
    sat = m_color.getSaturation();
    val = m_color.getValue();
  }
  else {
    hue = m_color.getHue();
    sat = (100.0 * u / umax);
    val = (100.0 - 100.0 * v / vmax);
  }

  return app::Color::fromHsv(
    MID(0.0, hue, 360.0),
    MID(0.0, sat, 100.0),
    MID(0.0, val, 100.0));
}

void ColorTintShadeTone::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  theme->drawRect(g, clientBounds(),
                  theme->parts.editorNormal().get(),
                  bgColor());

  gfx::Rect rc = clientChildrenBounds();
  if (rc.isEmpty())
    return;

  double hue = m_color.getHue();
  int umax, vmax;
  int huebar = getHueBarSize();
  umax = MAX(1, rc.w-1);
  vmax = MAX(1, rc.h-1-huebar);

  for (int y=0; y<rc.h-huebar; ++y) {
    for (int x=0; x<rc.w; ++x) {
      double sat = (100.0 * x / umax);
      double val = (100.0 - 100.0 * y / vmax);

      gfx::Color color = color_utils::color_for_ui(
        app::Color::fromHsv(
          hue,
          MID(0.0, sat, 100.0),
          MID(0.0, val, 100.0)));

      g->putPixel(color, rc.x+x, rc.y+y);
    }
  }

  if (huebar > 0) {
    for (int y=rc.h-huebar; y<rc.h; ++y) {
      for (int x=0; x<rc.w; ++x) {
        gfx::Color color = color_utils::color_for_ui(
          app::Color::fromHsv(
            (360.0 * x / rc.w), 100.0, 100.0));

        g->putPixel(color, rc.x+x, rc.y+y);
      }
    }
  }

  if (m_color.getType() != app::Color::MaskType) {
    double sat = m_color.getSaturation();
    double val = m_color.getValue();
    gfx::Point pos(rc.x + int(sat * rc.w / 100.0),
                   rc.y + int((100.0-val) * (rc.h-huebar) / 100.0));

    she::Surface* icon = theme->parts.colorWheelIndicator()->bitmap(0);
    g->drawColoredRgbaSurface(
      icon,
      val > 50.0 ? gfx::rgba(0, 0, 0): gfx::rgba(255, 255, 255),
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
      captureMouse();
      // Continue...

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

      if (msg->type() == kMouseDownMessage)
        m_capturedInHue = inHueBarArea(mouseMsg->position());

      app::Color color = pickColor(mouseMsg->position());
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
        ui::set_mouse_cursor(kEyedropperCursor);
        return true;
      }
      break;
    }

  }

  return Widget::onProcessMessage(msg);
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
