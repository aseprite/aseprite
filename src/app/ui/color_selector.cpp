// Aseprite
// Copyright (C) 2016-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_selector.h"

#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "base/scoped_value.h"
#include "she/surface.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"

#include <cmath>

namespace app {

using namespace app::skin;
using namespace ui;

ColorSelector::ColorSelector()
  : Widget(kGenericWidget)
  , m_lockColor(false)
  , m_capturedInBottom(false)
{
  setBorder(gfx::Border(3*ui::guiscale()));
}

void ColorSelector::selectColor(const app::Color& color)
{
  if (m_lockColor)
    return;

  m_color = color;
  invalidate();
}

app::Color ColorSelector::getColorByPosition(const gfx::Point& pos)
{
  gfx::Rect rc = childrenBounds();
  if (rc.isEmpty())
    return app::Color::fromMask();

  int u, v, umax, vmax;
  int barSize = getBottomBarSize();
  u = pos.x - rc.x;
  v = pos.y - rc.y;
  umax = MAX(1, rc.w-1);
  vmax = MAX(1, rc.h-1-barSize);

  if (( hasCapture() && m_capturedInBottom) ||
      (!hasCapture() && inBottomBarArea(pos)))
    return getBottomBarColor(u, umax);
  else
    return getMainAreaColor(u, umax, v, vmax);
}

void ColorSelector::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(32*ui::guiscale(), 32*ui::guiscale()));
}

bool ColorSelector::onProcessMessage(ui::Message* msg)
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
        m_capturedInBottom = inBottomBarArea(mouseMsg->position());

      app::Color color = getColorByPosition(mouseMsg->position());
      if (color != app::Color::fromMask()) {
        base::ScopedValue<bool> switcher(m_lockColor, subColorPicked(), false);

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
      app::Color color = getColorByPosition(mouseMsg->position());
      if (color.getType() != app::Color::MaskType) {
        ui::set_mouse_cursor(kCustomCursor, SkinTheme::instance()->cursors.eyedropper());
        return true;
      }
      break;
    }

    case kMouseWheelMessage:
      if (!hasCapture()) {
        double scale = 1.0;
        if (msg->shiftPressed() ||
            msg->ctrlPressed() ||
            msg->altPressed()) {
          scale = 15.0;
        }

        double newHue = m_color.getHsvHue()
          + scale*(+ static_cast<MouseMessage*>(msg)->wheelDelta().x
                   - static_cast<MouseMessage*>(msg)->wheelDelta().y);

        while (newHue < 0.0)
          newHue += 360.0;
        newHue = std::fmod(newHue, 360.0);

        if (newHue != m_color.getHsvHue()) {
          app::Color newColor =
            app::Color::fromHsv(
              newHue,
              m_color.getHsvSaturation(),
              m_color.getHsvValue());

          ColorChange(newColor, kButtonNone);
        }
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void ColorSelector::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  theme->drawRect(g, clientBounds(),
                  theme->parts.editorNormal().get(),
                  false);       // Do not fill the center

  gfx::Rect rc = clientChildrenBounds();
  if (rc.isEmpty())
    return;

  int barSize = getBottomBarSize();
  rc.h -= barSize;
  onPaintMainArea(g, rc);

  if (barSize > 0) {
    rc.y += rc.h;
    rc.h = barSize;
    onPaintBottomBar(g, rc);
  }
}

void ColorSelector::paintColorIndicator(ui::Graphics* g,
                                        const gfx::Point& pos,
                                        const bool white)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  she::Surface* icon = theme->parts.colorWheelIndicator()->bitmap(0);

  g->drawColoredRgbaSurface(
    icon,
    white ? gfx::rgba(255, 255, 255): gfx::rgba(0, 0, 0),
    pos.x-icon->width()/2,
    pos.y-icon->height()/2);
}

bool ColorSelector::inBottomBarArea(const gfx::Point& pos) const
{
  gfx::Rect rc = childrenBounds();
  if (rc.isEmpty() || !rc.contains(pos))
    return false;
  else
    return (pos.y >= rc.y+rc.h-getBottomBarSize());
}

int ColorSelector::getBottomBarSize() const
{
  gfx::Rect rc = clientChildrenBounds();
  int size = 8*guiscale();
  return rc.h < 2*size ? 0: size;
}

} // namespace app
