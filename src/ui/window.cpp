// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/window.h"

#include "gfx/size.h"
#include "ui/button.h"
#include "ui/graphics.h"
#include "ui/intern.h"
#include "ui/label.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/message_loop.h"
#include "ui/move_region.h"
#include "ui/resize_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace ui {

using namespace gfx;

namespace {

enum {
  WINDOW_NONE = 0,
  WINDOW_MOVE = 1,
  WINDOW_RESIZE_LEFT = 2,
  WINDOW_RESIZE_RIGHT = 4,
  WINDOW_RESIZE_TOP = 8,
  WINDOW_RESIZE_BOTTOM = 16,
};

gfx::Point clickedMousePos;
gfx::Rect* clickedWindowPos = nullptr;

class WindowTitleLabel : public Label {
public:
  WindowTitleLabel(const std::string& text) : Label(text) {
    setDecorative(true);
    setType(kWindowTitleLabelWidget);
    initTheme();
  }
};


// Controls the "X" button in a window to close it.
class WindowCloseButton : public ButtonBase {
public:
  WindowCloseButton()
    : ButtonBase("", kWindowCloseButtonWidget,
                 kButtonWidget, kButtonWidget) {
    setDecorative(true);
    initTheme();
  }

protected:

  void onClick(Event& ev) override {
    ButtonBase::onClick(ev);
    closeWindow();
  }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kSetCursorMessage:
        ui::set_mouse_cursor(kArrowCursor);
        return true;

      case kKeyDownMessage:
        if (window()->isForeground() &&
            static_cast<KeyMessage*>(msg)->scancode() == kKeyEsc) {
          setSelected(true);
          return true;
        }
        break;

      case kKeyUpMessage:
        if (window()->isForeground() &&
            static_cast<KeyMessage*>(msg)->scancode() == kKeyEsc) {
          if (isSelected()) {
            setSelected(false);
            closeWindow();
            return true;
          }
        }
        break;
    }

    return ButtonBase::onProcessMessage(msg);
  }
};

} // anonymous namespace

Window::Window(Type type, const std::string& text)
  : Widget(kWindowWidget)
  , m_closer(nullptr)
  , m_titleLabel(nullptr)
  , m_closeButton(nullptr)
  , m_isDesktop(type == DesktopWindow)
  , m_isMoveable(!m_isDesktop)
  , m_isSizeable(!m_isDesktop)
  , m_isOnTop(false)
  , m_isWantFocus(true)
  , m_isForeground(false)
  , m_isAutoRemap(true)
{
  setVisible(false);
  setAlign(LEFT | MIDDLE);
  if (type == WithTitleBar) {
    setText(text);
    addChild(m_closeButton = new WindowCloseButton);
  }

  initTheme();
}

Window::~Window()
{
  manager()->_closeWindow(this, isVisible());
}

void Window::setAutoRemap(bool state)
{
  m_isAutoRemap = state;
}

void Window::setMoveable(bool state)
{
  m_isMoveable = state;
}

void Window::setSizeable(bool state)
{
  m_isSizeable = state;
}

void Window::setOnTop(bool state)
{
  m_isOnTop = state;
}

void Window::setWantFocus(bool state)
{
  m_isWantFocus = state;
}

HitTest Window::hitTest(const gfx::Point& point)
{
  HitTestEvent ev(this, point, HitTestNowhere);
  onHitTest(ev);
  return ev.hit();
}

void Window::onClose(CloseEvent& ev)
{
  // Fire Close signal
  Close(ev);
}

void Window::onHitTest(HitTestEvent& ev)
{
  HitTest ht = HitTestNowhere;

  // If this window is not movable or we are not completely visible.
  if (!m_isMoveable) {
    ev.setHit(ht);
    return;
  }

  // TODO check why this is necessary, there should be a bug in
  // the manager where we are receiving mouse events and are not
  // the top most window.
  Widget* picked = manager()->pick(ev.point());
  if (picked &&
      picked != this &&
      picked->type() != kWindowTitleLabelWidget) {
    ev.setHit(ht);
    return;
  }

  int x = ev.point().x;
  int y = ev.point().y;
  gfx::Rect pos = bounds();
  gfx::Rect cpos = childrenBounds();

  // Move
  if ((hasText())
      && (((x >= cpos.x) &&
           (x < cpos.x2()) &&
           (y >= pos.y+border().bottom()) &&
           (y < cpos.y)))) {
    ht = HitTestCaption;
  }
  // Resize
  else if (m_isSizeable) {
    if ((x >= pos.x) && (x < cpos.x)) {
      if ((y >= pos.y) && (y < cpos.y))
        ht = HitTestBorderNW;
      else if ((y > cpos.y2()-1) && (y <= pos.y2()-1))
        ht = HitTestBorderSW;
      else
        ht = HitTestBorderW;
    }
    else if ((y >= pos.y) && (y < cpos.y)) {
      if ((x >= pos.x) && (x < cpos.x))
        ht = HitTestBorderNW;
      else if ((x > cpos.x2()-1) && (x <= pos.x2()-1))
        ht = HitTestBorderNE;
      else
        ht = HitTestBorderN;
    }
    else if ((x > cpos.x2()-1) && (x <= pos.x2()-1)) {
      if ((y >= pos.y) && (y < cpos.y))
        ht = HitTestBorderNE;
      else if ((y > cpos.y2()-1) && (y <= pos.y2()-1))
        ht = HitTestBorderSE;
      else
        ht = HitTestBorderE;
    }
    else if ((y > cpos.y2()-1) && (y <= pos.y2()-1)) {
      if ((x >= pos.x) && (x < cpos.x))
        ht = HitTestBorderSW;
      else if ((x > cpos.x2()-1) && (x <= pos.x2()-1))
        ht = HitTestBorderSE;
      else
        ht = HitTestBorderS;
    }
  }
  else {
    // Client area
    ht = HitTestClient;
  }

  ev.setHit(ht);
}

void Window::onWindowResize()
{
  // Do nothing
}

void Window::onWindowMovement()
{
  // Do nothing
}

void Window::remapWindow()
{
  if (m_isAutoRemap) {
    m_isAutoRemap = false;
    this->setVisible(true);
  }

  setBounds(Rect(Point(bounds().x, bounds().y),
                 sizeHint()));

  // load layout
  loadLayout();

  invalidate();
}

void Window::centerWindow()
{
  Widget* manager = this->manager();

  if (m_isAutoRemap)
    remapWindow();

  positionWindow(manager->bounds().w/2 - bounds().w/2,
                 manager->bounds().h/2 - bounds().h/2);
}

void Window::positionWindow(int x, int y)
{
  if (m_isAutoRemap)
    remapWindow();

  setBounds(Rect(x, y, bounds().w, bounds().h));

  invalidate();
}

void Window::moveWindow(const gfx::Rect& rect)
{
  moveWindow(rect, true);
}

void Window::openWindow()
{
  if (!parent()) {
    if (m_isAutoRemap)
      centerWindow();

    Manager::getDefault()->_openWindow(this);
  }
}

void Window::openWindowInForeground()
{
  m_isForeground = true;

  openWindow();

  MessageLoop loop(manager());
  while (!hasFlags(HIDDEN))
    loop.pumpMessages();

  m_isForeground = false;
}

void Window::closeWindow(Widget* closer)
{
  m_closer = closer;

  manager()->_closeWindow(this, true);

  // Close event
  CloseEvent ev(closer);
  onClose(ev);
}

bool Window::isTopLevel()
{
  Widget* manager = this->manager();
  if (!manager->children().empty())
    return (this == UI_FIRST_WIDGET(manager->children()));
  else
    return false;
}

bool Window::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      m_closer = NULL;
      break;

    case kCloseMessage:
      saveLayout();
      break;

    case kMouseDownMessage: {
      if (!m_isMoveable)
        break;

      clickedMousePos = static_cast<MouseMessage*>(msg)->position();
      m_hitTest = hitTest(clickedMousePos);

      if (m_hitTest != HitTestNowhere &&
          m_hitTest != HitTestClient) {
        if (clickedWindowPos == NULL)
          clickedWindowPos = new gfx::Rect(bounds());
        else
          *clickedWindowPos = bounds();

        captureMouse();
        return true;
      }
      else
        break;
    }

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();
        set_mouse_cursor(kArrowCursor);

        if (clickedWindowPos != NULL) {
          delete clickedWindowPos;
          clickedWindowPos = NULL;
        }

        m_hitTest = HitTestNowhere;
        return true;
      }
      break;

    case kMouseMoveMessage:
      if (!m_isMoveable)
        break;

      // Does it have the mouse captured?
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        // Reposition/resize
        if (m_hitTest == HitTestCaption) {
          int x = clickedWindowPos->x + (mousePos.x - clickedMousePos.x);
          int y = clickedWindowPos->y + (mousePos.y - clickedMousePos.y);
          moveWindow(gfx::Rect(x, y,
                               bounds().w,
                               bounds().h), true);
        }
        else {
          int x, y, w, h;

          w = clickedWindowPos->w;
          h = clickedWindowPos->h;

          bool hitLeft = (m_hitTest == HitTestBorderNW ||
                          m_hitTest == HitTestBorderW ||
                          m_hitTest == HitTestBorderSW);
          bool hitTop = (m_hitTest == HitTestBorderNW ||
                         m_hitTest == HitTestBorderN ||
                         m_hitTest == HitTestBorderNE);
          bool hitRight = (m_hitTest == HitTestBorderNE ||
                           m_hitTest == HitTestBorderE ||
                           m_hitTest == HitTestBorderSE);
          bool hitBottom = (m_hitTest == HitTestBorderSW ||
                            m_hitTest == HitTestBorderS ||
                            m_hitTest == HitTestBorderSE);

          if (hitLeft) {
            w += clickedMousePos.x - mousePos.x;
          }
          else if (hitRight) {
            w += mousePos.x - clickedMousePos.x;
          }

          if (hitTop) {
            h += (clickedMousePos.y - mousePos.y);
          }
          else if (hitBottom) {
            h += (mousePos.y - clickedMousePos.y);
          }

          limitSize(&w, &h);

          if ((bounds().w != w) ||
              (bounds().h != h)) {
            if (hitLeft)
              x = clickedWindowPos->x - (w - clickedWindowPos->w);
            else
              x = bounds().x;

            if (hitTop)
              y = clickedWindowPos->y - (h - clickedWindowPos->h);
            else
              y = bounds().y;

            moveWindow(gfx::Rect(x, y, w, h), false);
            invalidate();
          }
        }
      }
      break;

    case kSetCursorMessage:
      if (m_isMoveable) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        HitTest ht = hitTest(mousePos);
        CursorType cursor = kArrowCursor;

        switch (ht) {
          case HitTestCaption: cursor = kArrowCursor; break;
          case HitTestBorderNW: cursor = kSizeNWCursor; break;
          case HitTestBorderW: cursor = kSizeWCursor; break;
          case HitTestBorderSW: cursor = kSizeSWCursor; break;
          case HitTestBorderNE: cursor = kSizeNECursor; break;
          case HitTestBorderE: cursor = kSizeECursor; break;
          case HitTestBorderSE: cursor = kSizeSECursor; break;
          case HitTestBorderN: cursor = kSizeNCursor; break;
          case HitTestBorderS: cursor = kSizeSCursor; break;
        }

        set_mouse_cursor(cursor);
        return true;
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void Window::onResize(ResizeEvent& ev)
{
  windowSetPosition(ev.bounds());
}

void Window::onSizeHint(SizeHintEvent& ev)
{
  Widget* manager = this->manager();

  if (m_isDesktop) {
    Rect cpos = manager->childrenBounds();
    ev.setSizeHint(cpos.w, cpos.h);
  }
  else {
    Size maxSize(0, 0);
    Size reqSize;

    for (auto child : children()) {
      if (!child->isDecorative()) {
        reqSize = child->sizeHint();

        maxSize.w = MAX(maxSize.w, reqSize.w);
        maxSize.h = MAX(maxSize.h, reqSize.h);
      }
    }

    if (m_titleLabel)
      maxSize.w = MAX(maxSize.w, m_titleLabel->sizeHint().w);

    ev.setSizeHint(maxSize.w + border().width(),
                   maxSize.h + border().height());
  }
}

void Window::onBroadcastMouseMessage(WidgetsList& targets)
{
  targets.push_back(this);

  // Continue sending the message to siblings windows until a desktop
  // or foreground window.
  if (isForeground() || isDesktop())
    return;

  Widget* sibling = nextSibling();
  if (sibling)
    sibling->broadcastMouseMessage(targets);
}

void Window::onSetText()
{
  onBuildTitleLabel();
  Widget::onSetText();
  initTheme();
}

void Window::onBuildTitleLabel()
{
  if (text().empty()) {
    if (m_titleLabel) {
      removeChild(m_titleLabel);
      delete m_titleLabel;
      m_titleLabel = nullptr;
    }
  }
  else {
    if (!m_titleLabel) {
      m_titleLabel = new WindowTitleLabel(text());
      addChild(m_titleLabel);
    }
    else {
      m_titleLabel->setText(text());
      m_titleLabel->setBounds(
        gfx::Rect(m_titleLabel->bounds()).setSize(
          m_titleLabel->sizeHint()));
    }
  }
}

void Window::windowSetPosition(const gfx::Rect& rect)
{
  // Copy the new position rectangle
  setBoundsQuietly(rect);
  Rect cpos = childrenBounds();

  // Set all the children to the same "cpos"
  for (auto child : children()) {
    if (child->isDecorative())
      child->setDecorativeWidgetBounds();
    else
      child->setBounds(cpos);
  }

  onWindowResize();
}

void Window::limitSize(int* w, int* h)
{
  *w = MAX(*w, border().width());
  *h = MAX(*h, border().height());
}

void Window::moveWindow(const gfx::Rect& rect, bool use_blit)
{
#define FLAGS (DrawableRegionFlags)(kCutTopWindows | kUseChildArea)

  Manager* manager = this->manager();

  // Discard enqueued kWinMoveMessage for this window because we are
  // going to send a new kWinMoveMessage with the latest window
  // bounds.
  manager->removeMessagesFor(this, kWinMoveMessage);

  // Get the window's current position
  Rect old_pos = bounds();
  int dx = rect.x - old_pos.x;
  int dy = rect.y - old_pos.y;

  // Get the manager's current position
  Rect man_pos = manager->bounds();

  // Send a kWinMoveMessage message to the window
  Message* msg = new Message(kWinMoveMessage);
  msg->setRecipient(this);
  manager->enqueueMessage(msg);

  // Get the region & the drawable region of the window
  Region oldDrawableRegion;
  getDrawableRegion(oldDrawableRegion, FLAGS);

  // If the size of the window changes...
  if (old_pos.w != rect.w || old_pos.h != rect.h) {
    // We have to change the position of all children.
    windowSetPosition(rect);
  }
  else {
    // We can just displace all the widgets by a delta (new_position -
    // old_position)...
    offsetWidgets(dx, dy);
  }

  // Get the new drawable region of the window (it's new because we
  // moved the window to "rect")
  Region newDrawableRegion;
  getDrawableRegion(newDrawableRegion, FLAGS);

  // First of all, we have to find the manager region to invalidate,
  // it's the old window drawable region without the new window
  // drawable region.
  Region invalidManagerRegion;
  invalidManagerRegion.createSubtraction(
    oldDrawableRegion,
    newDrawableRegion);

  // In second place, we have to setup the window invalid region...

  // If "use_blit" isn't activated, we have to redraw the whole window
  // (sending kPaintMessage messages) in the new drawable region
  if (!use_blit) {
    invalidateRegion(newDrawableRegion);
  }
  // If "use_blit" is activated, we can move the old drawable to the
  // new position (to redraw as little as possible).
  else {
    Region reg1;
    reg1 = newDrawableRegion;
    reg1.offset(-dx, -dy);

    Region moveableRegion;
    moveableRegion.createIntersection(oldDrawableRegion, reg1);

    // Move the window's graphics
    ScreenGraphics g;
    hide_mouse_cursor();
    {
      IntersectClip clip(&g, man_pos);
      if (clip) {
        ui::move_region(manager, moveableRegion, dx, dy);
      }
    }
    show_mouse_cursor();

    reg1.createSubtraction(reg1, moveableRegion);
    reg1.offset(dx, dy);
    invalidateRegion(reg1);
  }

  manager->invalidateRegion(invalidManagerRegion);

  onWindowMovement();
}

} // namespace ui
