// Aseprite UI Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/size.h"
#include "ui/graphics.h"
#include "ui/intern.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace ui {

using namespace gfx;

PopupWindow::PopupWindow(const std::string& text,
                         const ClickBehavior clickBehavior,
                         const EnterBehavior enterBehavior,
                         const bool withCloseButton)
  : Window(text.empty() ? WithoutTitleBar: WithTitleBar, text)
  , m_clickBehavior(clickBehavior)
  , m_enterBehavior(enterBehavior)
  , m_filtering(false)
  , m_fixed(false)
{
  setSizeable(false);
  setMoveable(false);
  setWantFocus(false);
  setAlign(LEFT | TOP);

  if (!withCloseButton) {
    // Remove close button
    for (auto child : children()) {
      if (child->type() == kWindowCloseButtonWidget) {
        delete child;
        break;
      }
    }
  }

  initTheme();
}

PopupWindow::~PopupWindow()
{
  stopFilteringMessages();
}

void PopupWindow::setHotRegion(const gfx::Region& region)
{
  startFilteringMessages();

  m_hotRegion = region;
}

void PopupWindow::setClickBehavior(ClickBehavior behavior)
{
  m_clickBehavior = behavior;
}

void PopupWindow::setEnterBehavior(EnterBehavior behavior)
{
  m_enterBehavior = behavior;
}

void PopupWindow::makeFloating()
{
  stopFilteringMessages();
  setMoveable(true);
  m_fixed = false;

  onMakeFloating();
}

void PopupWindow::makeFixed()
{
  startFilteringMessages();
  setMoveable(false);
  m_fixed = true;

  onMakeFixed();
}

bool PopupWindow::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    // There are cases where startFilteringMessages() is called when a
    // kCloseMessage for this same PopupWindow is enqueued. Processing
    // the kOpenMessage we ensure that the popup will be filtering
    // messages if it's needed when it's visible (as kCloseMessage and
    // kOpenMessage must be enqueued in the correct order).
    case kOpenMessage:
      if (!isMoveable())
        startFilteringMessages();
      break;

    case kCloseMessage:
      stopFilteringMessages();
      break;

    case kMouseLeaveMessage:
      if (m_hotRegion.isEmpty() && m_fixed)
        closeWindow(nullptr);
      break;

    case kKeyDownMessage:
      if (m_filtering) {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        if (scancode == kKeyEsc)
          closeWindow(nullptr);

        if (m_enterBehavior == EnterBehavior::CloseOnEnter &&
            (scancode == kKeyEnter ||
             scancode == kKeyEnterPad)) {
          closeWindow(this);
          return true;
        }

        // If the message came from a filter, we don't send it back to
        // the default Window processing (which will send the message
        // to the Manager). In this way, the focused children can
        // process the kKeyDownMessage.
        if (msg->fromFilter())
          return false;
      }
      break;

    case kMouseDownMessage:
      if (m_filtering &&
          manager()->getTopWindow() == this) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        switch (m_clickBehavior) {

          // If the user click outside the window, we have to close
          // the tooltip window.
          case ClickBehavior::CloseOnClickInOtherWindow: {
            Widget* picked = pick(mousePos);
            if (!picked || picked->window() != this) {
              closeWindow(nullptr);
            }
            break;
          }

          case ClickBehavior::CloseOnClickOutsideHotRegion:
            if (!m_hotRegion.contains(mousePos)) {
              closeWindow(nullptr);
            }
            break;
        }
      }
      break;

    case kMouseMoveMessage:
      if (m_fixed &&
          !m_hotRegion.isEmpty() &&
          manager()->getCapture() == nullptr) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        // If the mouse is outside the hot-region we have to close the
        // window.
        if (!m_hotRegion.contains(mousePos))
          closeWindow(nullptr);
      }
      break;

  }

  return Window::onProcessMessage(msg);
}

void PopupWindow::onHitTest(HitTestEvent& ev)
{
  Window::onHitTest(ev);

  Widget* picked = manager()->pick(ev.point());
  if (picked) {
    WidgetType type = picked->type();
    if (type == kWindowWidget && picked == this) {
      if (isSizeable() && (ev.hit() == HitTestBorderNW ||
                           ev.hit() == HitTestBorderN ||
                           ev.hit() == HitTestBorderNE ||
                           ev.hit() == HitTestBorderE ||
                           ev.hit() == HitTestBorderSE ||
                           ev.hit() == HitTestBorderS ||
                           ev.hit() == HitTestBorderSW ||
                           ev.hit() == HitTestBorderW)) {
        // Use the hit value from Window::onHitTest()
        return;
      }
      else {
        ev.setHit(isMoveable() ? HitTestCaption: HitTestClient);
      }
    }
    else if (type == kBoxWidget ||
             type == kLabelWidget ||
             type == kLinkLabelWidget ||
             type == kGridWidget ||
             type == kSeparatorWidget) {
      ev.setHit(isMoveable() ? HitTestCaption: HitTestClient);
    }
  }
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

void PopupWindow::onMakeFloating()
{
  // Do nothing
}

void PopupWindow::onMakeFixed()
{
  // Do nothing
}

TransparentPopupWindow::TransparentPopupWindow(ClickBehavior clickBehavior)
  : PopupWindow("", clickBehavior)
{
  setTransparent(true);
  initTheme();
}

void TransparentPopupWindow::onInitTheme(InitThemeEvent& ev)
{
  PopupWindow::onInitTheme(ev);
  // TODO fix this, if we use alpha=0 (gfx::ColorNone), we get
  // "window_face" color as background the transparent popup window.
  //setBgColor(gfx::ColorNone);
  setBgColor(gfx::rgba(0, 0, 0, 1));
}

} // namespace ui
