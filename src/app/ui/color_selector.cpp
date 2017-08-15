// Aseprite
// Copyright (C) 2016-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_selector.h"

#include "app/color_utils.h"
#include "app/modules/gfx.h"
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
  , m_capturedInAlpha(false)
{
  initTheme();
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

  const int u = pos.x - rc.x;
  const int umax = MAX(1, rc.w-1);

  const gfx::Rect bottomBarBounds = this->bottomBarBounds();
  if (( hasCapture() && m_capturedInBottom) ||
      (!hasCapture() && bottomBarBounds.contains(pos)))
    return getBottomBarColor(u, umax);

  const gfx::Rect alphaBarBounds = this->alphaBarBounds();
  if (( hasCapture() && m_capturedInAlpha) ||
      (!hasCapture() && alphaBarBounds.contains(pos)))
    return getAlphaBarColor(u, umax);

  const int v = pos.y - rc.y;
  const int vmax = MAX(1, rc.h-bottomBarBounds.h-alphaBarBounds.h-1);
  return getMainAreaColor(u, umax,
                          v, vmax);
}

app::Color ColorSelector::getAlphaBarColor(const int u, const int umax)
{
  int alpha = (255 * u / umax);
  app::Color color = m_color;
  color.setAlpha(MID(0, alpha, 255));
  return color;
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
      const gfx::Point pos = mouseMsg->position();

      if (msg->type() == kMouseDownMessage) {
        m_capturedInBottom = bottomBarBounds().contains(pos);
        m_capturedInAlpha = alphaBarBounds().contains(pos);
      }

      app::Color color = getColorByPosition(pos);
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
        m_capturedInBottom = false;
        m_capturedInAlpha = false;
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
              m_color.getHsvValue(),
              m_color.getAlpha());

          ColorChange(newColor, kButtonNone);
        }
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void ColorSelector::onInitTheme(ui::InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);
  setBorder(gfx::Border(3*ui::guiscale()));
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

  gfx::Rect bottomBarBounds = this->bottomBarBounds();
  gfx::Rect alphaBarBounds = this->alphaBarBounds();
  rc.h -= bottomBarBounds.h + alphaBarBounds.h;
  onPaintMainArea(g, rc);

  if (!bottomBarBounds.isEmpty()) {
    bottomBarBounds.offset(-bounds().origin());
    onPaintBottomBar(g, bottomBarBounds);
  }

  if (!alphaBarBounds.isEmpty()) {
    alphaBarBounds.offset(-bounds().origin());
    onPaintAlphaBar(g, alphaBarBounds);
  }
}

void ColorSelector::onPaintAlphaBar(ui::Graphics* g, const gfx::Rect& rc)
{
  draw_alpha_slider(g, rc, m_color);

  const double lit = m_color.getHslLightness();
  const int alpha = m_color.getAlpha();
  const gfx::Point pos(rc.x + int(rc.w * alpha / 255),
                       rc.y + rc.h/2);
  paintColorIndicator(g, pos, lit < 0.5);
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

gfx::Rect ColorSelector::bottomBarBounds() const
{
  const gfx::Rect rc = childrenBounds();
  const int size = 8*guiscale();      // TODO 8 should be configurable in theme.xml
  if (rc.h > 2*size) {
    if (rc.h > 3*size)          // Alpha bar is visible too
      return gfx::Rect(rc.x, rc.y2()-size*2, rc.w, size);
    else
      return gfx::Rect(rc.x, rc.y2()-size, rc.w, size);
  }
  else
    return gfx::Rect();
}

gfx::Rect ColorSelector::alphaBarBounds() const
{
  const gfx::Rect rc = childrenBounds();
  const int size = 8*guiscale();      // TODO 8 should be configurable in theme.xml
  if (rc.h > 3*size)
    return gfx::Rect(rc.x, rc.y2()-size, rc.w, size);
  else
    return gfx::Rect();
}

} // namespace app
