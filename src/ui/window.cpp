// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#define REDRAW_MOVEMENT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>

#include "gfx/size.h"
#include "ui/intern.h"
#include "ui/preferred_size_event.h"
#include "ui/ui.h"

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

static void displace_widgets(Widget* widget, int x, int y);

Window::Window(bool desktop, const char* text)
  : Widget(kWindowWidget)
{
  m_killer = NULL;
  m_isDesktop = desktop;
  m_isMoveable = !desktop;
  m_isSizeable = !desktop;
  m_isOnTop = false;
  m_isWantFocus = true;
  m_isForeground = false;
  m_isAutoRemap = true;

  setVisible(false);
  setText(text);
  setAlign(JI_LEFT | JI_MIDDLE);

  initTheme();
}

Window::~Window()
{
  getManager()->_closeWindow(this, false);
}

Widget* Window::getKiller()
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
  while (!getChildren().empty())
    delete getChildren().front();
}

void Window::onClose(CloseEvent& ev)
{
  // Fire Close signal
  Close(ev);
}

void Window::onHitTest(HitTestEvent& ev)
{
  HitTest ht = HitTestNowhere;

  if (!m_isMoveable) {
    ev.setHit(ht);
    return;
  }

  int x = ev.getPoint().x;
  int y = ev.getPoint().y;
  gfx::Rect pos = getBounds();
  gfx::Rect cpos = getChildrenBounds();

  // Move
  if ((this->hasText())
      && (((x >= cpos.x) &&
           (x < cpos.x2()) &&
           (y >= pos.y+this->border_width.b) &&
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

void Window::remapWindow()
{
  if (m_isAutoRemap) {
    m_isAutoRemap = false;
    this->setVisible(true);
  }

  setBounds(Rect(Point(this->rc->x1, this->rc->y1),
                 getPreferredSize()));

  // load layout
  loadLayout();

  invalidate();
}

void Window::centerWindow()
{
  Widget* manager = getManager();

  if (m_isAutoRemap)
    this->remapWindow();

  positionWindow(jrect_w(manager->rc)/2 - jrect_w(this->rc)/2,
                 jrect_h(manager->rc)/2 - jrect_h(this->rc)/2);
}

void Window::positionWindow(int x, int y)
{
  if (m_isAutoRemap)
    remapWindow();

  setBounds(Rect(x, y, jrect_w(this->rc), jrect_h(this->rc)));

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
  openWindow();

  MessageLoop loop(getManager());

  m_isForeground = true;

  while (!(this->flags & JI_HIDDEN))
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
  if (!manager->getChildren().empty())
    return (this == UI_FIRST_WIDGET(manager->getChildren()));
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
        jmouse_set_cursor(kArrowCursor);

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
                               jrect_w(this->rc),
                               jrect_h(this->rc)), true);
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

          if ((jrect_w(this->rc) != w) ||
              (jrect_h(this->rc) != h)) {
            if (hitLeft)
              x = clickedWindowPos->x - (w - clickedWindowPos->w);
            else
              x = this->rc->x1;

            if (hitTop)
              y = clickedWindowPos->y - (h - clickedWindowPos->h);
            else
              y = this->rc->y1;

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

          case HitTestCaption:
            cursor = kArrowCursor;
            break;

          case HitTestBorderNW:
            cursor = kSizeTLCursor;
            break;

          case HitTestBorderW:
            cursor = kSizeLCursor;
            break;

          case HitTestBorderSW:
            cursor = kSizeBLCursor;
            break;

          case HitTestBorderNE:
            cursor = kSizeTRCursor;
            break;

          case HitTestBorderE:
            cursor = kSizeRCursor;
            break;

          case HitTestBorderSE:
            cursor = kSizeBRCursor;
            break;

          case HitTestBorderN:
            cursor = kSizeTCursor;
            break;

          case HitTestBorderS:
            cursor = kSizeBCursor;
            break;

        }

        jmouse_set_cursor(cursor);
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

void Window::onPreferredSize(PreferredSizeEvent& ev)
{
  Widget* manager = getManager();

  if (m_isDesktop) {
    Rect cpos = manager->getChildrenBounds();
    ev.setPreferredSize(cpos.w, cpos.h);
  }
  else {
    Size maxSize(0, 0);
    Size reqSize;

    UI_FOREACH_WIDGET(getChildren(), it) {
      Widget* child = *it;

      if (!child->isDecorative()) {
        reqSize = child->getPreferredSize();

        maxSize.w = MAX(maxSize.w, reqSize.w);
        maxSize.h = MAX(maxSize.h, reqSize.h);
      }
    }

    if (this->hasText())
      maxSize.w = MAX(maxSize.w, jwidget_get_text_length(this));

    ev.setPreferredSize(this->border_width.l + maxSize.w + this->border_width.r,
                        this->border_width.t + maxSize.h + this->border_width.b);
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
  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;

    if (child->isDecorative())
      child->setDecorativeWidgetBounds();
    else
      child->setBounds(cpos);
  }
}

void Window::limitSize(int *w, int *h)
{
  *w = MAX(*w, this->border_width.l+this->border_width.r);
  *h = MAX(*h, this->border_width.t+this->border_width.b);
}

void Window::moveWindow(const gfx::Rect& rect, bool use_blit)
{
#define FLAGS (DrawableRegionFlags)(kCutTopWindows | kUseChildArea)

  Manager* manager = getManager();
  Region old_drawable_region;
  Region new_drawable_region;
  Region manager_refresh_region; // A region to refresh the manager later
  Region window_refresh_region;  // A new region to refresh the window later
  Message* msg;

  manager->dispatchMessages();

  // Get the window's current position
  Rect old_pos = getBounds();

  // Get the manager's current position
  Rect man_pos = manager->getBounds();

  // Send a kWinMoveMessage message to the window
  msg = new Message(kWinMoveMessage);
  msg->addRecipient(this);
  manager->enqueueMessage(msg);

  // Get the region & the drawable region of the window
  getDrawableRegion(old_drawable_region, FLAGS);

  // If the size of the window changes...
  if (old_pos.w != rect.w ||
      old_pos.h != rect.h) {
    // We have to change the position of all children.
    windowSetPosition(rect);
  }
  else {
    // We can just displace all the widgets by a delta (new_position -
    // old_position)...
    displace_widgets(this,
                     rect.x - old_pos.x,
                     rect.y - old_pos.y);
  }

  // Get the new drawable region of the window (it's new because we
  // moved the window to "rect")
  getDrawableRegion(new_drawable_region, FLAGS);

  // First of all, we have to refresh the manager in the old window's
  // drawable region, but we have to substract the new window's
  // drawable region.
  manager_refresh_region.createSubtraction(old_drawable_region,
                                           new_drawable_region);

  // In second place, we have to setup the window's refresh region...

  // If "use_blit" isn't activated, we have to redraw the whole window
  // (sending kPaintMessage messages) in the new drawable region
  if (!use_blit) {
    window_refresh_region = new_drawable_region;
  }
  // If "use_blit" is activated, we can move the old drawable to the
  // new position (to redraw as little as possible)
  else {
    Region reg1;
    Region moveable_region;

    // Add a region to draw areas which were outside of the screen
    reg1 = new_drawable_region;
    reg1.offset(old_pos.x - this->rc->x1,
                old_pos.y - this->rc->y1);
    moveable_region.createIntersection(old_drawable_region, reg1);

    reg1.createSubtraction(reg1, moveable_region);
    reg1.offset(this->rc->x1 - old_pos.x,
                this->rc->y1 - old_pos.y);
    window_refresh_region.createUnion(window_refresh_region, reg1);

    // Move the window's graphics
    jmouse_hide();
    set_clip_rect(ji_screen,
                  man_pos.x, man_pos.y, man_pos.x2()-1, man_pos.y2()-1);

    ji_move_region(moveable_region,
                   this->rc->x1 - old_pos.x,
                   this->rc->y1 - old_pos.y);
    set_clip_rect(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);
    jmouse_show();
  }

  manager->invalidateDisplayRegion(manager_refresh_region);
  invalidateRegion(window_refresh_region);
}

static void displace_widgets(Widget* widget, int x, int y)
{
  jrect_displace(widget->rc, x, y);

  UI_FOREACH_WIDGET(widget->getChildren(), it)
    displace_widgets((*it), x, y);
}

} // namespace ui
