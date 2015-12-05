// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
  : Widget(kGenericWidget)
{
  setAlign(HORIZONTAL);
  setBorder(gfx::Border(3*ui::guiscale()));
}

ColorSpectrum::~ColorSpectrum()
{
}

app::Color ColorSpectrum::pickColor(const gfx::Point& pos) const
{
  gfx::Rect rc = childrenBounds();
  if (rc.isEmpty() || !rc.contains(pos))
    return app::Color::fromMask();

  int vmid = (align() & HORIZONTAL ? rc.h/2 : rc.w/2);
  vmid = MAX(1, vmid);

  int u, v, umax;
  if (align() & HORIZONTAL) {
    u = pos.x - rc.x;
    v = pos.y - rc.y;
    umax = MAX(1, rc.w-1);
  }
  else {
    u = pos.y - rc.y;
    v = pos.x - rc.x;
    umax = MAX(1, rc.h-1);
  }

  int hue = 360 * u / umax;
  int sat = (v < vmid ? 100 * v / vmid : 100);
  int val = (v < vmid ? 100 : 100-(100 * (v-vmid) / vmid));

  return app::Color::fromHsv(
    MID(0, hue, 360),
    MID(0, sat, 100),
    MID(0, val, 100));
}

void ColorSpectrum::selectColor(const app::Color& color)
{
  m_color = color;
  invalidate();
}

void ColorSpectrum::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(32*ui::guiscale(), 32*ui::guiscale()));
}

void ColorSpectrum::onResize(ui::ResizeEvent& ev)
{
  Widget::onResize(ev);
}

void ColorSpectrum::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  theme->drawRect(g, clientBounds(),
                  theme->parts.editorNormal().get(),
                  bgColor());

  gfx::Rect rc = clientChildrenBounds();
  if (rc.isEmpty())
    return;

  int vmid = (align() & HORIZONTAL ? rc.h/2 : rc.w/2);
  vmid = MAX(1, vmid);

  for (int y=0; y<rc.h; ++y) {
    for (int x=0; x<rc.w; ++x) {
      int u, v, umax;
      if (align() & HORIZONTAL) {
        u = x;
        v = y;
        umax = MAX(1, rc.w-1);
      }
      else {
        u = y;
        v = x;
        umax = MAX(1, rc.h-1);
      }

      int hue = 360 * u / umax;
      int sat = (v < vmid ? 100 * v / vmid : 100);
      int val = (v < vmid ? 100 : 100-(100 * (v-vmid) / vmid));

      gfx::Color color = color_utils::color_for_ui(
        app::Color::fromHsv(
          MID(0, hue, 360),
          MID(0, sat, 100),
          MID(0, val, 100)));

      g->putPixel(color, rc.x+x, rc.y+y);
    }
  }

  if (m_color.getType() != app::Color::MaskType) {
    int hue = m_color.getHue();
    int sat = m_color.getSaturation();
    int val = m_color.getValue();
    int lit = (200 - sat) * val / 200;
    gfx::Point pos(rc.x + hue * rc.w / 360,
                   rc.y + rc.h - (lit * rc.h / 100));

    she::Surface* icon = theme->parts.colorWheelIndicator()->bitmap(0);
    g->drawColoredRgbaSurface(
      icon,
      lit > 50 ? gfx::rgba(0, 0, 0): gfx::rgba(255, 255, 255),
      pos.x-icon->width()/2,
      pos.y-icon->height()/2);
  }
}

bool ColorSpectrum::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage:
      captureMouse();
      // Continue...

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

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

} // namespace app
