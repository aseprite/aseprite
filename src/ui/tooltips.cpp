// Aseprite UI Library
// Copyright (C) 2018-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/tooltips.h"

#include "gfx/size.h"
#include "ui/fit_bounds.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/scale.h"
#include "ui/system.h"
#include "ui/textbox.h"
#include "ui/theme.h"

#include <string>

static constexpr int kDefaultTooltipDelayMsecs = 300;

namespace ui {

using namespace gfx;

TooltipManager::TooltipManager() : m_delay(kDefaultTooltipDelayMsecs)
{
  Manager* manager = Manager::getDefault();
  manager->addMessageFilter(kMouseEnterMessage, this);
  manager->addMessageFilter(kKeyDownMessage, this);
  manager->addMessageFilter(kMouseDownMessage, this);
  manager->addMessageFilter(kMouseLeaveMessage, this);
  setVisible(false);
}

TooltipManager::~TooltipManager()
{
  Manager* manager = Manager::getDefault();
  manager->removeMessageFilterFor(this);
}

void TooltipManager::addTooltipFor(Widget* widget, const std::string& text, int arrowAlign)
{
  ASSERT(widget);

  // This can happen if we add a tooltip to a label, we have to start listening mouse events.
  if (widget->hasFlags(IGNORE_MOUSE))
    widget->disableFlags(IGNORE_MOUSE);

  m_tips[widget] = TipInfo(text, arrowAlign);
}

void TooltipManager::removeTooltipFor(Widget* widget)
{
  const auto it = m_tips.find(widget);
  if (it != m_tips.end())
    m_tips.erase(it);
}

void TooltipManager::setDelay(int delay)
{
  m_delay = delay;
  if (m_timer)
    m_timer->setInterval(m_delay);
}

std::string TooltipManager::getTooltipFor(Widget* widget)
{
  const auto it = m_tips.find(widget);
  if (it != m_tips.end())
    return m_tips[widget].text;

  return {};
}

void TooltipManager::closeTooltip()
{
  if (m_tipWindow) {
    m_tipWindow->closeWindow(nullptr);
    m_tipWindow.reset();
  }

  if (m_timer)
    m_timer->stop();
}

void TooltipManager::showAreaTooltip(Widget* parent,
                                     const gfx::Rect& bounds,
                                     const std::string& text,
                                     int arrowAlign)
{
  // This will be called from MouseMove so we need to deduplicate to let the timer run
  if (m_target.isArea && m_target.bounds == bounds && (m_timer->isRunning() || m_tipWindow))
    return;

  m_target.isArea = true;
  showTooltip(parent, bounds, TipInfo(text, arrowAlign));
}

bool TooltipManager::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kMouseEnterMessage: {
      // Tooltips are only for widgets that can directly get the mouse
      // (get the kMouseEnterMessage directly).
      if (Widget* widget = msg->recipient()) {
        if (m_delay <= 0)
          return false;

        const auto it = m_tips.find(widget);
        if (it != m_tips.end()) {
          m_target.isArea = false;
          showTooltip(it->first, it->first->bounds(), it->second);
        }
      }
      return false;
    }

    case kKeyDownMessage:
    case kMouseDownMessage:
    case kMouseLeaveMessage: closeTooltip(); return false;
    case kMouseMoveMessage:  {
      if (m_tipWindow && m_target.isArea) {
        const MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        if (!m_target.bounds.contains(mouseMsg->position())) {
          closeTooltip();
          Manager::getDefault()->removeMessageFilter(kMouseMoveMessage, this);
          return false;
        }
      }
      break;
    }
  }

  return Widget::onProcessMessage(msg);
}

void TooltipManager::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);
  if (m_tipWindow)
    m_tipWindow->initTheme();
}

void TooltipManager::onTick()
{
  if (!m_tipWindow) {
    m_tipWindow = std::make_unique<TipWindow>(m_target.tipInfo.text);

    int arrowAlign = m_target.tipInfo.arrowAlign;
    gfx::Rect target = m_target.bounds;
    if (!arrowAlign)
      target.setOrigin(m_target.widget->mousePosInDisplay() + 12 * guiscale());

    ui::Display* targetDisplay = m_target.widget->display();

    if (m_tipWindow->pointAt(arrowAlign, target, targetDisplay)) {
      m_tipWindow->openWindow();
      m_tipWindow->adjustTargetFrom(targetDisplay);
    }
    else {
      // No enough room for the tooltip
      m_tipWindow.reset();
      m_timer->stop();
    }

    if (m_target.isArea) {
      Manager::getDefault()->addMessageFilter(kMouseMoveMessage, this);
    }
  }
  m_timer->stop();
}

void TooltipManager::showTooltip(Widget* widget, const gfx::Rect& bounds, const TipInfo& info)
{
  m_target.widget = widget;
  m_target.bounds = bounds;
  m_target.tipInfo = info;

  if (m_timer == nullptr) {
    m_timer = std::make_unique<Timer>(m_delay, this);
    m_timer->Tick.connect(&TooltipManager::onTick, this);
  }

  m_timer->start();
}

// TipWindow

TipWindow::TipWindow(const std::string& text)
  // Put an empty string in the ctor so the window label isn't build
  : PopupWindow("", ClickBehavior::CloseOnClickInOtherWindow)
  , m_arrowStyle(nullptr)
  , m_arrowAlign(0)
  , m_closeOnKeyDown(true)
  , m_textBox(new TextBox("", LEFT | TOP))
{
  setTransparent(true);

  // Here we build our own custimized label for the window
  // (a text box).
  m_textBox->setVisible(false);
  addChild(m_textBox);
  setText(text);

  makeFixed();
  initTheme();
}

void TipWindow::setCloseOnKeyDown(bool state)
{
  m_closeOnKeyDown = state;
}

bool TipWindow::pointAt(int arrowAlign, const gfx::Rect& target, const ui::Display* display)
{
  // TODO merge this code with the new ui::fit_bounds() algorithm

  m_target = target;
  m_arrowAlign = arrowAlign;

  remapWindow();

  int x = target.x;
  int y = target.y;
  int w = bounds().w;
  int h = bounds().h;

  os::Window* nativeParentWindow = display->nativeWindow();

  int trycount = 0;
  for (; trycount < 4; ++trycount) {
    switch (arrowAlign) {
      case TOP | LEFT:
        x = m_target.x + m_target.w;
        y = m_target.y + m_target.h;
        break;
      case TOP | RIGHT:
        x = m_target.x - w;
        y = m_target.y + m_target.h;
        break;
      case BOTTOM | LEFT:
        x = m_target.x + m_target.w;
        y = m_target.y - h;
        break;
      case BOTTOM | RIGHT:
        x = m_target.x - w;
        y = m_target.y - h;
        break;
      case TOP:
        x = m_target.x + m_target.w / 2 - w / 2;
        y = m_target.y + m_target.h;
        break;
      case BOTTOM:
        x = m_target.x + m_target.w / 2 - w / 2;
        y = m_target.y - h;
        break;
      case LEFT:
        x = m_target.x + m_target.w;
        y = m_target.y + m_target.h / 2 - h / 2;
        break;
      case RIGHT:
        x = m_target.x - w;
        y = m_target.y + m_target.h / 2 - h / 2;
        break;
    }

    if (get_multiple_displays()) {
      const gfx::Rect waBounds = nativeParentWindow->screen()->workarea();
      gfx::Point pt = nativeParentWindow->pointToScreen(gfx::Point(x, y));
      pt.x = std::clamp(pt.x, waBounds.x, std::max(waBounds.x, waBounds.x2() - w));
      pt.y = std::clamp(pt.y, waBounds.y, std::max(waBounds.y, waBounds.y2() - h));
      pt = nativeParentWindow->pointFromScreen(pt);
      x = pt.x;
      y = pt.y;
    }
    else {
      const gfx::Rect displayBounds = display->bounds();
      x = std::clamp(x, displayBounds.x, std::max(displayBounds.x, displayBounds.x2() - w));
      y = std::clamp(y, displayBounds.y, std::max(displayBounds.y, displayBounds.y2() - h));
    }

    if (m_target.intersects(gfx::Rect(x, y, w, h))) {
      switch (trycount) {
        case 0:
        case 2:
          // Switch position
          if (arrowAlign & (TOP | BOTTOM))
            arrowAlign ^= TOP | BOTTOM;
          if (arrowAlign & (LEFT | RIGHT))
            arrowAlign ^= LEFT | RIGHT;
          break;
        case 1:
          // Rotate positions
          if (arrowAlign & (TOP | LEFT))
            arrowAlign ^= TOP | LEFT;
          if (arrowAlign & (BOTTOM | RIGHT))
            arrowAlign ^= BOTTOM | RIGHT;
          break;
      }
    }
    else {
      m_arrowAlign = arrowAlign;
      ui::fit_bounds(display, this, gfx::Rect(x, y, w, h));
      break;
    }
  }

  return (trycount < 4);
}

void TipWindow::adjustTargetFrom(const ui::Display* targetDisplay)
{
  // Convert the target relative to this window coordinates
  if (get_multiple_displays()) {
    gfx::Point pt = m_target.origin();
    pt = targetDisplay->nativeWindow()->pointToScreen(pt);
    pt = display()->nativeWindow()->pointFromScreen(pt);
    m_target.setOrigin(pt);
  }
  else {
    m_target.offset(-bounds().origin());
  }
}

bool TipWindow::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kKeyDownMessage:
      if (m_closeOnKeyDown && static_cast<KeyMessage*>(msg)->scancode() < kKeyFirstModifierScancode)
        closeWindow(nullptr);
      break;
  }

  return PopupWindow::onProcessMessage(msg);
}

void TipWindow::onPaint(PaintEvent& ev)
{
  theme()->paintTooltip(ev.graphics(),
                        this,
                        style(),
                        arrowStyle(),
                        clientBounds(),
                        arrowAlign(),
                        target());
}

void TipWindow::onBuildTitleLabel()
{
  if (!text().empty()) {
    m_textBox->setVisible(true);
    m_textBox->setText(text());
  }
  else
    m_textBox->setVisible(false);
}

} // namespace ui
