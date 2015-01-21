// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/size.h"
#include "ui/graphics.h"
#include "ui/intern.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace ui {

using namespace gfx;

PopupWindow::PopupWindow(const std::string& text, ClickBehavior clickBehavior)
  : Window(WithTitleBar, text)
  , m_clickBehavior(clickBehavior)
  , m_filtering(false)
{
  setSizeable(false);
  setMoveable(false);
  setWantFocus(false);
  setAlign(JI_LEFT | JI_TOP);

  removeDecorativeWidgets();

  initTheme();
  noBorderNoChildSpacing();
}

PopupWindow::~PopupWindow()
{
  stopFilteringMessages();
}

/**
 * @param region The new hot-region. This pointer is holded by the @a widget.
 * So you cannot destroy it after calling this routine.
 */
void PopupWindow::setHotRegion(const gfx::Region& region)
{
  startFilteringMessages();

  m_hotRegion = region;
}

void PopupWindow::makeFloating()
{
  stopFilteringMessages();
  setMoveable(true);
}

void PopupWindow::makeFixed()
{
  startFilteringMessages();
  setMoveable(false);
}

bool PopupWindow::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kCloseMessage:
      stopFilteringMessages();
      break;

    case kMouseLeaveMessage:
      if (m_hotRegion.isEmpty() && !isMoveable())
        closeWindow(NULL);
      break;

    case kKeyDownMessage:
      if (m_filtering) {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();
        
        if (scancode == kKeyEsc ||
            scancode == kKeyEnter ||
            scancode == kKeyEnterPad) {
          closeWindow(NULL);
        }

        // If we are filtering messages we don't propagate key-events
        // to other widgets. As we're a popup window and we're
        // filtering messages, the user shouldn't be able to start
        // other actions pressing keyboard shortcuts.
        return false;
      }
      break;

    case kMouseDownMessage:
      if (m_filtering) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        switch (m_clickBehavior) {

          // If the user click outside the window, we have to close
          // the tooltip window.
          case kCloseOnClickInOtherWindow: {
            Widget* picked = pick(mousePos);
            if (!picked || picked->getRoot() != this) {
              closeWindow(NULL);
            }
            break;
          }

          case kCloseOnClickOutsideHotRegion:
            if (!m_hotRegion.contains(mousePos)) {
              closeWindow(NULL);
            }
            break;
        }
      }
      break;

    case kMouseMoveMessage:
      if (!isMoveable() &&
          !m_hotRegion.isEmpty() &&
          getManager()->getCapture() == NULL) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        // If the mouse is outside the hot-region we have to close the
        // window.
        if (!m_hotRegion.contains(mousePos))
          closeWindow(NULL);
      }
      break;

  }

  return Window::onProcessMessage(msg);
}

void PopupWindow::onPreferredSize(PreferredSizeEvent& ev)
{
  ScreenGraphics g;
  g.setFont(getFont());
  Size resultSize(0, 0);

  if (hasText())
    resultSize = g.fitString(getText(),
                             (getClientBounds() - getBorder()).w,
                             getAlign());

  resultSize.w += border_width.l + border_width.r;
  resultSize.h += border_width.t + border_width.b;

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

void PopupWindow::onPaint(PaintEvent& ev)
{
  getTheme()->paintPopupWindow(ev);
}

void PopupWindow::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);

  this->border_width.l = 3 * guiscale();
  this->border_width.t = 3 * guiscale();
  this->border_width.r = 3 * guiscale();
  this->border_width.b = 3 * guiscale();
}

void PopupWindow::onHitTest(HitTestEvent& ev)
{
  Widget* picked = getManager()->pick(ev.getPoint());
  if (picked) {
    WidgetType type = picked->getType();
    if ((type == kWindowWidget && picked == this) ||
        type == kBoxWidget ||
        type == kLabelWidget ||
        type == kGridWidget ||
        type == kSeparatorWidget) {
      ev.setHit(HitTestCaption);
      return;
    }
  }
  Window::onHitTest(ev);
}

void PopupWindow::startFilteringMessages()
{
  if (!m_filtering) {
    m_filtering = true;

    Manager* manager = Manager::getDefault();
    manager->addMessageFilter(kMouseMoveMessage, this);
    manager->addMessageFilter(kMouseDownMessage, this);
    manager->addMessageFilter(kKeyDownMessage, this);
  }
}

void PopupWindow::stopFilteringMessages()
{
  if (m_filtering) {
    m_filtering = false;

    Manager* manager = Manager::getDefault();
    manager->removeMessageFilter(kMouseMoveMessage, this);
    manager->removeMessageFilter(kMouseDownMessage, this);
    manager->removeMessageFilter(kKeyDownMessage, this);
  }
}

} // namespace ui
