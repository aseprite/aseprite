// Aseprite UI Library
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/window.h"

#include "gfx/size.h"
#include "ui/graphics.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/message_loop.h"
#include "ui/move_region.h"
#include "ui/size_hint_event.h"
#include "ui/resize_event.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace ui {

using namespace gfx;

enum {
  WINDOW_NONE = 0,
  WINDOW_MOVE = 1,
  WINDOW_RESIZE_LEFT = 2,
  WINDOW_RESIZE_RIGHT = 4,
  WINDOW_RESIZE_TOP = 8,
  WINDOW_RESIZE_BOTTOM = 16,
};

static gfx::Point clickedMousePos;
static gfx::Rect* clickedWindowPos = NULL;

Window::Window(Type type, const std::string& text)
  : Widget(kWindowWidget)
{
  m_killer = NULL;
  m_isDesktop = (type == DesktopWindow);
  m_isMoveable = !m_isDesktop;
  m_isSizeable = !m_isDesktop;
  m_isOnTop = false;
  m_isWantFocus = true;
  m_isForeground = false;
  m_isAutoRemap = true;

  setVisible(false);
  setAlign(LEFT | MIDDLE);
  if (type == WithTitleBar)
    setText(text);

  initTheme();
}

Window::~Window()
{
  getManager()->_closeWindow(this, false);
}

Widget* Window::getKiller() const
{
  return m_killer;
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
  return ev.getHit();
}

void Window::removeDecorativeWidgets()
{
  while (!children().empty())
    delete children().front();
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
  if (!m_isMoveable ||
      // TODO check why this is necessary, there should be a bug in
      // the manager where we are receiving mouse events and are not
      // the top most window.
      getManager()->pick(ev.getPoint()) != this) {
    ev.setHit(ht);
    return;
  }

  int x = ev.getPoint().x;
  int y = ev.getPoint().y;
  gfx::Rect pos = getBounds();
  gfx::Rect cpos = getChildrenBounds();

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

void Window::remapWindow()
{
  if (m_isAutoRemap) {
    m_isAutoRemap = false;
    this->setVisible(true);
  }

  setBounds(Rect(Point(getBounds().x, getBounds().y),
                 sizeHint()));

  // load layout
  loadLayout();

  invalidate();
}

void Window::centerWindow()
{
  Widget* manager = getManager();

  if (m_isAutoRemap)
    remapWindow();

  positionWindow(manager->getBounds().w/2 - getBounds().w/2,
                 manager->getBounds().h/2 - getBounds().h/2);
}

void Window::positionWindow(int x, int y)
{
  if (m_isAutoRemap)
    remapWindow();

  setBounds(Rect(x, y, getBounds().w, getBounds().h));

  invalidate();
}

void Window::moveWindow(const gfx::Rect& rect)
{
  moveWindow(rect, true);
}

void Window::openWindow()
{
  if (!getParent()) {
    if (m_isAutoRemap)
      centerWindow();

    Manager::getDefault()->_openWindow(this);
  }
}

void Window::openWindowInForeground()
{
  m_isForeground = true;

  openWindow();

  MessageLoop loop(getManager());
  while (!hasFlags(HIDDEN))
    loop.pumpMessages();

  m_isForeground = false;
}

void Window::closeWindow(Widget* killer)
{
  m_killer = killer;

  getManager()->_closeWindow(this, true);

  // Close event
  CloseEvent ev(killer);
  onClose(ev);
}

bool Window::isTopLevel()
{
  Widget* manager = getManager();
  if (!manager->children().empty())
    return (this == UI_FIRST_WIDGET(manager->children()));
  else
    return false;
}

bool Window::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      m_killer = NULL;
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
          clickedWindowPos = new gfx::Rect(getBounds());
        else
          *clickedWindowPos = getBounds();

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
                               getBounds().w,
                               getBounds().h), true);
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

          if ((getBounds().w != w) ||
              (getBounds().h != h)) {
            if (hitLeft)
              x = clickedWindowPos->x - (w - clickedWindowPos->w);
            else
              x = getBounds().x;

            if (hitTop)
              y = clickedWindowPos->y - (h - clickedWindowPos->h);
            else
              y = getBounds().y;

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
  windowSetPosition(ev.getBounds());
}

void Window::onSizeHint(SizeHintEvent& ev)
{
  Widget* manager = getManager();

  if (m_isDesktop) {
    Rect cpos = manager->getChildrenBounds();
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

    if (hasText())
      maxSize.w = MAX(maxSize.w, getTextWidth());

    ev.setSizeHint(maxSize.w + border().width(),
                   maxSize.h + border().height());
  }
}

void Window::onPaint(PaintEvent& ev)
{
  getTheme()->paintWindow(ev);
}

void Window::onBroadcastMouseMessage(WidgetsList& targets)
{
  targets.push_back(this);

  // Continue sending the message to siblings windows until a desktop
  // or foreground window.
  if (isForeground() || isDesktop())
    return;

  Widget* sibling = getNextSibling();
  if (sibling)
    sibling->broadcastMouseMessage(targets);
}

void Window::onSetText()
{
  Widget::onSetText();
  initTheme();
}

void Window::windowSetPosition(const gfx::Rect& rect)
{
  // Copy the new position rectangle
  setBoundsQuietly(rect);
  Rect cpos = getChildrenBounds();

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

  Manager* manager = getManager();
  Message* msg;

  manager->dispatchMessages();

  // Get the window's current position
  Rect old_pos = getBounds();
  int dx = rect.x - old_pos.x;
  int dy = rect.y - old_pos.y;

  // Get the manager's current position
  Rect man_pos = manager->getBounds();

  // Send a kWinMoveMessage message to the window
  msg = new Message(kWinMoveMessage);
  msg->addRecipient(this);
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

  manager->invalidateDisplayRegion(invalidManagerRegion);
}

} // namespace ui
