// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>
#include <string>

#include "base/unique_ptr.h"
#include "gfx/size.h"
#include "ui/graphics.h"
#include "ui/gui.h"
#include "ui/intern.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"

static const int kTooltipDelayMsecs = 300;

using namespace gfx;

namespace ui {

TooltipManager::TooltipManager()
  : Widget(JI_WIDGET)
{
  Manager* manager = Manager::getDefault();
  manager->addMessageFilter(JM_MOUSEENTER, this);
  manager->addMessageFilter(JM_KEYPRESSED, this);
  manager->addMessageFilter(JM_BUTTONPRESSED, this);
  manager->addMessageFilter(JM_MOUSELEAVE, this);

  setVisible(false);
}

TooltipManager::~TooltipManager()
{
  Manager* manager = Manager::getDefault();
  manager->removeMessageFilterFor(this);
}

void TooltipManager::addTooltipFor(Widget* widget, const char* text, int arrowAlign)
{
  m_tips[widget] = TipInfo(text, arrowAlign);
}

bool TooltipManager::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_MOUSEENTER: {
      UI_FOREACH_WIDGET(*msg->any.widgets, itWidget) {
        Tips::iterator it = m_tips.find(*itWidget);
        if (it != m_tips.end()) {
          m_target.widget = it->first;
          m_target.tipInfo = it->second;

          if (m_timer == NULL) {
            m_timer.reset(new Timer(kTooltipDelayMsecs, this));
            m_timer->Tick.connect(&TooltipManager::onTick, this);
          }

          m_timer->start();
        }
      }
      return false;
    }

    case JM_KEYPRESSED:
    case JM_BUTTONPRESSED:
    case JM_MOUSELEAVE:
      if (m_tipWindow) {
        m_tipWindow->closeWindow(NULL);
        m_tipWindow.reset();
      }

      if (m_timer)
        m_timer->stop();

      return false;
  }

  return Widget::onProcessMessage(msg);
}

void TooltipManager::onTick()
{
  if (!m_tipWindow) {
    m_tipWindow.reset(new TipWindow(m_target.tipInfo.text.c_str(), true));
    gfx::Rect bounds = m_target.widget->getBounds();
    int x = jmouse_x(0)+12*jguiscale();
    int y = jmouse_y(0)+12*jguiscale();
    int w, h;

    m_tipWindow->setArrowAlign(m_target.tipInfo.arrowAlign);
    m_tipWindow->remapWindow();

    w = jrect_w(m_tipWindow->rc);
    h = jrect_h(m_tipWindow->rc);

    switch (m_target.tipInfo.arrowAlign) {
      case JI_TOP | JI_LEFT:
        x = bounds.x + bounds.w;
        y = bounds.y + bounds.h;
        break;
      case JI_TOP | JI_RIGHT:
        x = bounds.x - w;
        y = bounds.y + bounds.h;
        break;
      case JI_BOTTOM | JI_LEFT:
        x = bounds.x + bounds.w;
        y = bounds.y - h;
        break;
      case JI_BOTTOM | JI_RIGHT:
        x = bounds.x - w;
        y = bounds.y - h;
        break;
      case JI_TOP:
        x = bounds.x + bounds.w/2 - w/2;
        y = bounds.y + bounds.h;
        break;
      case JI_BOTTOM:
        x = bounds.x + bounds.w/2 - w/2;
        y = bounds.y - h;
        break;
      case JI_LEFT:
        x = bounds.x + bounds.w;
        y = bounds.y + bounds.h/2 - h/2;
        break;
      case JI_RIGHT:
        x = bounds.x - w;
        y = bounds.y + bounds.h/2 - h/2;
        break;
    }

    // if (x+w > JI_SCREEN_W) {
    //   x = jmouse_x(0) - w - 4*jguiscale();
    //   y = jmouse_y(0);
    // }

    m_tipWindow->positionWindow(MID(0, x, JI_SCREEN_W-w),
                                MID(0, y, JI_SCREEN_H-h));
    m_tipWindow->openWindow();
  }
  m_timer->stop();
}

// TipWindow

TipWindow::TipWindow(const char *text, bool close_on_buttonpressed)
  : Window(false, text)
{
  m_close_on_buttonpressed = close_on_buttonpressed;
  m_filtering = false;
  m_arrowAlign = 0;

  setSizeable(false);
  setMoveable(false);
  setWantFocus(false);
  setAlign(JI_LEFT | JI_TOP);

  removeDecorativeWidgets();

  initTheme();
}

TipWindow::~TipWindow()
{
  if (m_filtering) {
    m_filtering = false;
    getManager()->removeMessageFilter(JM_MOTION, this);
    getManager()->removeMessageFilter(JM_BUTTONPRESSED, this);
    getManager()->removeMessageFilter(JM_KEYPRESSED, this);
  }
}

void TipWindow::setHotRegion(const Region& region)
{
  if (!m_filtering) {
    m_filtering = true;
    getManager()->addMessageFilter(JM_MOTION, this);
    getManager()->addMessageFilter(JM_BUTTONPRESSED, this);
    getManager()->addMessageFilter(JM_KEYPRESSED, this);
  }

  m_hotRegion = region;
}

int TipWindow::getArrowAlign() const
{
  return m_arrowAlign;
}

void TipWindow::setArrowAlign(int arrowAlign)
{
  m_arrowAlign = arrowAlign;
}

bool TipWindow::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      if (m_filtering) {
        m_filtering = false;
        getManager()->removeMessageFilter(JM_MOTION, this);
        getManager()->removeMessageFilter(JM_BUTTONPRESSED, this);
        getManager()->removeMessageFilter(JM_KEYPRESSED, this);
      }
      break;

    case JM_MOUSELEAVE:
      if (m_hotRegion.isEmpty())
        closeWindow(NULL);
      break;

    case JM_KEYPRESSED:
      if (m_filtering && msg->key.scancode < KEY_MODIFIERS)
        this->closeWindow(NULL);
      break;

    case JM_BUTTONPRESSED:
      /* if the user click outside the window, we have to close the
         tooltip window */
      if (m_filtering) {
        Widget* picked = this->pick(msg->mouse.x, msg->mouse.y);
        if (!picked || picked->getRoot() != this) {
          this->closeWindow(NULL);
        }
      }

      /* this is used when the user click inside a small text
         tooltip */
      if (m_close_on_buttonpressed)
        this->closeWindow(NULL);
      break;

    case JM_MOTION:
      if (!m_hotRegion.isEmpty() &&
          getManager()->getCapture() == NULL) {
        // If the mouse is outside the hot-region we have to close the window
        if (!m_hotRegion.contains(Point(msg->mouse.x, msg->mouse.y))) {
          closeWindow(NULL);
        }
      }
      break;

  }

  return Window::onProcessMessage(msg);
}

void TipWindow::onPreferredSize(PreferredSizeEvent& ev)
{
  ScreenGraphics g;
  g.setFont(getFont());
  Size resultSize = g.fitString(getText(),
                                (getClientBounds() - getBorder()).w,
                                getAlign());

  resultSize.w += this->border_width.l + this->border_width.r;
  resultSize.h += this->border_width.t + this->border_width.b;

  if (!getChildren().empty()) {
    Size maxSize(0, 0);
    Size reqSize;

    UI_FOREACH_WIDGET(getChildren(), it) {
      Widget* child = *it;

      reqSize = child->getPreferredSize();

      maxSize.w = MAX(maxSize.w, reqSize.w);
      maxSize.h = MAX(maxSize.h, reqSize.h);
    }

    resultSize.w = MAX(resultSize.w, border_width.l + maxSize.w + border_width.r);
    resultSize.h += maxSize.h;
  }

  ev.setPreferredSize(resultSize);
}

void TipWindow::onInitTheme(InitThemeEvent& ev)
{
  Window::onInitTheme(ev);

  this->border_width.l = 6 * jguiscale();
  this->border_width.t = 6 * jguiscale();
  this->border_width.r = 6 * jguiscale();
  this->border_width.b = 7 * jguiscale();

  // Setup the background color.
  setBgColor(ui::rgba(255, 255, 200));
}

void TipWindow::onPaint(PaintEvent& ev)
{
  getTheme()->paintTooltip(ev);
}

} // namespace ui
