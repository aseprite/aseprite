// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#define REDRAW_MOVEMENT

#include "config.h"

#include <allegro.h>

#include "gfx/size.h"
#include "ui/gui.h"
#include "ui/intern.h"
#include "ui/preferred_size_event.h"

using namespace gfx;

namespace ui {

enum {
  WINDOW_NONE = 0,
  WINDOW_MOVE = 1,
  WINDOW_RESIZE_LEFT = 2,
  WINDOW_RESIZE_RIGHT = 4,
  WINDOW_RESIZE_TOP = 8,
  WINDOW_RESIZE_BOTTOM = 16,
};

static JRect click_pos = NULL;
static int press_x, press_y;

static void displace_widgets(Widget* widget, int x, int y);

Window::Window(bool desktop, const char* text)
  : Widget(JI_WINDOW)
{
  m_killer = NULL;
  m_is_desktop = desktop;
  m_is_moveable = !desktop;
  m_is_sizeable = !desktop;
  m_is_ontop = false;
  m_is_wantfocus = true;
  m_is_foreground = false;
  m_is_autoremap = true;

  setVisible(false);
  setText(text);
  setAlign(JI_LEFT | JI_MIDDLE);

  initTheme();
}

Window::~Window()
{
  getManager()->_closeWindow(this, false);
}

Widget* Window::get_killer()
{
  return m_killer;
}

void Window::set_autoremap(bool state)
{
  m_is_autoremap = state;
}

void Window::set_moveable(bool state)
{
  m_is_moveable = state;
}

void Window::set_sizeable(bool state)
{
  m_is_sizeable = state;
}

void Window::set_ontop(bool state)
{
  m_is_ontop = state;
}

void Window::set_wantfocus(bool state)
{
  m_is_wantfocus = state;
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

  if (!m_is_moveable) {
    ev.setHit(ht);
    return;
  }

  int x = ev.getPoint().x;
  int y = ev.getPoint().y;
  JRect pos = jwidget_get_rect(this);
  JRect cpos = jwidget_get_child_rect(this);

  // Move
  if ((this->hasText())
      && (((x >= cpos->x1) &&
           (x < cpos->x2) &&
           (y >= pos->y1+this->border_width.b) &&
           (y < cpos->y1)))) {
    ht = HitTestCaption;
  }
  // Resize
  else if (m_is_sizeable) {
    if ((x >= pos->x1) && (x < cpos->x1)) {
      if ((y >= pos->y1) && (y < cpos->y1))
        ht = HitTestBorderNW;
      else if ((y > cpos->y2-1) && (y <= pos->y2-1))
        ht = HitTestBorderSW;
      else
        ht = HitTestBorderW;
    }
    else if ((y >= pos->y1) && (y < cpos->y1)) {
      if ((x >= pos->x1) && (x < cpos->x1))
        ht = HitTestBorderNW;
      else if ((x > cpos->x2-1) && (x <= pos->x2-1))
        ht = HitTestBorderNE;
      else
        ht = HitTestBorderN;
    }
    else if ((x > cpos->x2-1) && (x <= pos->x2-1)) {
      if ((y >= pos->y1) && (y < cpos->y1))
        ht = HitTestBorderNE;
      else if ((y > cpos->y2-1) && (y <= pos->y2-1))
        ht = HitTestBorderSE;
      else
        ht = HitTestBorderE;
    }
    else if ((y > cpos->y2-1) && (y <= pos->y2-1)) {
      if ((x >= pos->x1) && (x < cpos->x1))
        ht = HitTestBorderSW;
      else if ((x > cpos->x2-1) && (x <= pos->x2-1))
        ht = HitTestBorderSE;
      else
        ht = HitTestBorderS;
    }
  }
  else {
    // Client area
    ht = HitTestClient;
  }

  jrect_free(pos);
  jrect_free(cpos);

  ev.setHit(ht);
}

void Window::remap_window()
{
  Size reqSize;
  JRect rect;

  if (m_is_autoremap) {
    m_is_autoremap = false;
    this->setVisible(true);
  }

  reqSize = this->getPreferredSize();

  rect = jrect_new(this->rc->x1, this->rc->y1,
                   this->rc->x1+reqSize.w,
                   this->rc->y1+reqSize.h);
  jwidget_set_rect(this, rect);
  jrect_free(rect);

  invalidate();
}

void Window::center_window()
{
  Widget* manager = getManager();

  if (m_is_autoremap)
    this->remap_window();

  position_window(jrect_w(manager->rc)/2 - jrect_w(this->rc)/2,
                  jrect_h(manager->rc)/2 - jrect_h(this->rc)/2);
}

void Window::position_window(int x, int y)
{
  JRect rect;

  if (m_is_autoremap)
    remap_window();

  rect = jrect_new(x, y, x+jrect_w(this->rc), y+jrect_h(this->rc));
  jwidget_set_rect(this, rect);
  jrect_free(rect);

  invalidate();
}

void Window::move_window(JRect rect)
{
  move_window(rect, true);
}

void Window::openWindow()
{
  if (!getParent()) {
    if (m_is_autoremap)
      center_window();

    Manager::getDefault()->_openWindow(this);
  }
}

void Window::openWindowInForeground()
{
  openWindow();

  MessageLoop loop(getManager());

  m_is_foreground = true;

  while (!(this->flags & JI_HIDDEN))
    loop.pumpMessages();

  m_is_foreground = false;
}

void Window::closeWindow(Widget* killer)
{
  m_killer = killer;

  getManager()->_closeWindow(this, true);

  // Close event
  CloseEvent ev(killer);
  onClose(ev);
}

bool Window::is_toplevel()
{
  Widget* manager = getManager();
  if (!manager->getChildren().empty())
    return (this == UI_FIRST_WIDGET(manager->getChildren()));
  else
    return false;
}

bool Window::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      this->window_set_position(&msg->setpos.rect);
      return true;

    case JM_OPEN:
      m_killer = NULL;
      break;

    case JM_BUTTONPRESSED: {
      if (!m_is_moveable)
        break;

      press_x = msg->mouse.x;
      press_y = msg->mouse.y;
      m_hitTest = hitTest(gfx::Point(press_x, press_y));

      if (m_hitTest != HitTestNowhere &&
          m_hitTest != HitTestClient) {
        if (click_pos == NULL)
          click_pos = jrect_new_copy(this->rc);
        else
          jrect_copy(click_pos, this->rc);

        captureMouse();
        return true;
      }
      else
        break;
    }

    case JM_BUTTONRELEASED:
      if (hasCapture()) {
        releaseMouse();
        jmouse_set_cursor(JI_CURSOR_NORMAL);

        if (click_pos != NULL) {
          jrect_free(click_pos);
          click_pos = NULL;
        }

        m_hitTest = HitTestNowhere;
        return true;
      }
      break;

    case JM_MOTION:
      if (!m_is_moveable)
        break;

      // Does it have the mouse captured?
      if (hasCapture()) {
        // Reposition/resize
        if (m_hitTest == HitTestCaption) {
          int x = click_pos->x1 + (msg->mouse.x - press_x);
          int y = click_pos->y1 + (msg->mouse.y - press_y);
          JRect rect = jrect_new(x, y,
                                 x+jrect_w(this->rc),
                                 y+jrect_h(this->rc));
          this->move_window(rect, true);
          jrect_free(rect);
        }
        else {
          int x, y, w, h;

          w = jrect_w(click_pos);
          h = jrect_h(click_pos);

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
            w += press_x - msg->mouse.x;
          }
          else if (hitRight) {
            w += msg->mouse.x - press_x;
          }

          if (hitTop) {
            h += (press_y - msg->mouse.y);
          }
          else if (hitBottom) {
            h += (msg->mouse.y - press_y);
          }

          this->limit_size(&w, &h);

          if ((jrect_w(this->rc) != w) ||
              (jrect_h(this->rc) != h)) {
            if (hitLeft)
              x = click_pos->x1 - (w - jrect_w(click_pos));
            else
              x = this->rc->x1;

            if (hitTop)
              y = click_pos->y1 - (h - jrect_h(click_pos));
            else
              y = this->rc->y1;

            {
              JRect rect = jrect_new(x, y, x+w, y+h);
              this->move_window(rect, false);
              jrect_free(rect);

              invalidate();
            }
          }
        }
      }
      break;

    case JM_SETCURSOR:
      if (m_is_moveable) {
        HitTest ht = hitTest(gfx::Point(msg->mouse.x, msg->mouse.y));
        int cursor = JI_CURSOR_NORMAL;

        switch (ht) {

          case HitTestCaption:
            cursor = JI_CURSOR_NORMAL;
            break;

          case HitTestBorderNW:
            cursor = JI_CURSOR_SIZE_TL;
            break;

          case HitTestBorderW:
            cursor = JI_CURSOR_SIZE_L;
            break;

          case HitTestBorderSW:
            cursor = JI_CURSOR_SIZE_BL;
            break;

          case HitTestBorderNE:
            cursor = JI_CURSOR_SIZE_TR;
            break;

          case HitTestBorderE:
            cursor = JI_CURSOR_SIZE_R;
            break;

          case HitTestBorderSE:
            cursor = JI_CURSOR_SIZE_BR;
            break;

          case HitTestBorderN:
            cursor = JI_CURSOR_SIZE_T;
            break;

          case HitTestBorderS:
            cursor = JI_CURSOR_SIZE_B;
            break;

        }

        jmouse_set_cursor(cursor);
        return true;
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void Window::onPreferredSize(PreferredSizeEvent& ev)
{
  Widget* manager = getManager();

  if (m_is_desktop) {
    JRect cpos = jwidget_get_child_rect(manager);
    ev.setPreferredSize(jrect_w(cpos),
                        jrect_h(cpos));
    jrect_free(cpos);
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
  if (is_foreground() || is_desktop())
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

void Window::window_set_position(JRect rect)
{
  /* copy the new position rectangle */
  jrect_copy(this->rc, rect);
  JRect cpos = jwidget_get_child_rect(this);

  /* set all the children to the same "child_pos" */
  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;

    if (child->isDecorative())
      child->getTheme()->map_decorative_widget(child);
    else
      jwidget_set_rect(child, cpos);
  }

  jrect_free(cpos);
}

void Window::limit_size(int *w, int *h)
{
  *w = MAX(*w, this->border_width.l+this->border_width.r);
  *h = MAX(*h, this->border_width.t+this->border_width.b);
}

void Window::move_window(JRect rect, bool use_blit)
{
#define FLAGS JI_GDR_CUTTOPWINDOWS | JI_GDR_USECHILDAREA

  Manager* manager = getManager();
  JRegion old_drawable_region;
  JRegion new_drawable_region;
  JRegion manager_refresh_region;
  JRegion window_refresh_region;
  JRect old_pos;
  JRect man_pos;
  Message* msg;

  manager->dispatchMessages();

  /* get the window's current position */
  old_pos = jrect_new_copy(this->rc);

  /* get the manager's current position */
  man_pos = jwidget_get_rect(manager);

  /* sent a JM_WINMOVE message to the window */
  msg = jmessage_new(JM_WINMOVE);
  jmessage_add_dest(msg, this);
  manager->enqueueMessage(msg);

  /* get the region & the drawable region of the window */
  old_drawable_region = jwidget_get_drawable_region(this, FLAGS);

  /* if the size of the window changes... */
  if (jrect_w(old_pos) != jrect_w(rect) ||
      jrect_h(old_pos) != jrect_h(rect)) {
    /* we have to change the whole positions sending JM_SETPOS
       messages... */
    window_set_position(rect);
  }
  else {
    /* we can just displace all the widgets
       by a delta (new_position - old_position)... */
    displace_widgets(this,
                     rect->x1 - old_pos->x1,
                     rect->y1 - old_pos->y1);
  }

  /* get the new drawable region of the window (it's new because we
     moved the window to "rect") */
  new_drawable_region = jwidget_get_drawable_region(this, FLAGS);

  /* create a new region to refresh the manager later */
  manager_refresh_region = jregion_new(NULL, 0);

  /* create a new region to refresh the window later */
  window_refresh_region = jregion_new(NULL, 0);

  /* first of all, we have to refresh the manager in the old window's
     drawable region... */
  jregion_copy(manager_refresh_region, old_drawable_region);

  /* ...but we have to substract the new window's drawable region (and
     that is all for the manager's refresh region) */
  jregion_subtract(manager_refresh_region, manager_refresh_region,
                   new_drawable_region);

  /* now we have to setup the window's refresh region... */

  /* if "use_blit" isn't activated, we have to redraw the whole window
     (sending JM_DRAW messages) in the new drawable region */
  if (!use_blit) {
    jregion_copy(window_refresh_region, new_drawable_region);
  }
  /* if "use_blit" is activated, we can move the old drawable to the
     new position (to redraw as little as possible) */
  else {
    JRegion reg1 = jregion_new(NULL, 0);
    JRegion moveable_region = jregion_new(NULL, 0);

    /* add a region to draw areas which were outside of the screen */
    jregion_copy(reg1, new_drawable_region);
    jregion_translate(reg1,
                      old_pos->x1 - this->rc->x1,
                      old_pos->y1 - this->rc->y1);
    jregion_intersect(moveable_region, old_drawable_region, reg1);

    jregion_subtract(reg1, reg1, moveable_region);
    jregion_translate(reg1,
                      this->rc->x1 - old_pos->x1,
                      this->rc->y1 - old_pos->y1);
    jregion_union(window_refresh_region, window_refresh_region, reg1);

    /* move the window's graphics */
    jmouse_hide();
    set_clip_rect(ji_screen,
                  man_pos->x1, man_pos->y1, man_pos->x2-1, man_pos->y2-1);

    ji_move_region(moveable_region,
                   this->rc->x1 - old_pos->x1,
                   this->rc->y1 - old_pos->y1);
    set_clip_rect(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);
    jmouse_show();

    jregion_free(reg1);
    jregion_free(moveable_region);
  }

  manager->invalidateDisplayRegion(manager_refresh_region);
  invalidateRegion(window_refresh_region);

  jregion_free(old_drawable_region);
  jregion_free(new_drawable_region);
  jregion_free(manager_refresh_region);
  jregion_free(window_refresh_region);
  jrect_free(old_pos);
  jrect_free(man_pos);
}

static void displace_widgets(Widget* widget, int x, int y)
{
  jrect_displace(widget->rc, x, y);

  UI_FOREACH_WIDGET(widget->getChildren(), it)
    displace_widgets((*it), x, y);
}

} // namespace ui
