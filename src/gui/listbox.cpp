// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro/keyboard.h>

#include "gfx/point.h"
#include "gfx/size.h"
#include "gui/list.h"
#include "gui/manager.h"
#include "gui/message.h"
#include "gui/rect.h"
#include "gui/system.h"
#include "gui/theme.h"
#include "gui/view.h"
#include "gui/widget.h"

using namespace gfx;

static bool listbox_msg_proc(JWidget widget, Message* msg);
static void listbox_request_size(JWidget widget, int *w, int *h);
static void listbox_set_position(JWidget widget, JRect rect);
static void listbox_dirty_children(JWidget widget);

static bool listitem_msg_proc(JWidget widget, Message* msg);
static void listitem_request_size(JWidget widget, int *w, int *h);

JWidget jlistbox_new()
{
  JWidget widget = new Widget(JI_LISTBOX);

  jwidget_add_hook(widget, JI_LISTBOX, listbox_msg_proc, NULL);
  jwidget_focusrest(widget, true);
  widget->initTheme();

  return widget;
}

JWidget jlistitem_new(const char *text)
{
  JWidget widget = new Widget(JI_LISTITEM);

  jwidget_add_hook(widget, JI_LISTITEM, listitem_msg_proc, NULL);
  widget->setAlign(JI_LEFT | JI_MIDDLE);
  widget->setText(text);
  widget->initTheme();

  return widget;
}

JWidget jlistbox_get_selected_child(JWidget widget)
{
  JLink link;
  JI_LIST_FOR_EACH(widget->children, link) {
    if (((JWidget)link->data)->isSelected())
      return (JWidget)link->data;
  }
  return 0;
}

int jlistbox_get_selected_index(JWidget widget)
{
  JLink link;
  int i = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    if (((JWidget)link->data)->isSelected())
      return i;
    i++;
  }

  return -1;
}

void jlistbox_select_child(JWidget widget, JWidget listitem)
{
  JWidget child;
  JLink link;

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    if (child->isSelected()) {
      if ((listitem) && (child == listitem))
        return;

      child->setSelected(false);
    }
  }

  if (listitem) {
    View* view = View::getView(widget);

    listitem->setSelected(true);

    if (view) {
      gfx::Rect vp = view->getViewportBounds();
      gfx::Point scroll = view->getViewScroll();

      if (listitem->rc->y1 < vp.y)
          scroll.y = listitem->rc->y1 - widget->rc->y1;
      else if (listitem->rc->y1 > vp.y + vp.h - jrect_h(listitem->rc))
        scroll.y = (listitem->rc->y1 - widget->rc->y1
                    - vp.h + jrect_h(listitem->rc));

      view->setViewScroll(scroll);
    }
  }

  jwidget_emit_signal(widget, JI_SIGNAL_LISTBOX_CHANGE);
}

void jlistbox_select_index(JWidget widget, int index)
{
  JWidget child = reinterpret_cast<JWidget>(jlist_nth_data(widget->children, index));
  if (child)
    jlistbox_select_child(widget, child);
}

int jlistbox_get_items_count(JWidget widget)
{
  return jlist_length(widget->children);
}

/* setup the scroll to center the selected item in the viewport */
void jlistbox_center_scroll(JWidget widget)
{
  View* view = View::getView(widget);
  Widget* listitem = jlistbox_get_selected_child(widget);

  if (view && listitem) {
    gfx::Rect vp = view->getViewportBounds();
    gfx::Point scroll = view->getViewScroll();

    scroll.y = ((listitem->rc->y1 - widget->rc->y1)
                - vp.h/2 + jrect_h(listitem->rc)/2);

    view->setViewScroll(scroll);
  }
}

static bool listbox_msg_proc(JWidget widget, Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      listbox_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS:
      listbox_set_position(widget, &msg->setpos.rect);
      return true;

    case JM_DRAW:
      widget->getTheme()->draw_listbox(widget, &msg->draw.rect);
      return true;

    case JM_DIRTYCHILDREN:
      listbox_dirty_children(widget);
      return true;

    case JM_OPEN:
      jlistbox_center_scroll(widget);
      break;

    case JM_BUTTONPRESSED:
      widget->captureMouse();

    case JM_MOTION:
      if (widget->hasCapture()) {
        int select = jlistbox_get_selected_index(widget);
        View* view = View::getView(widget);
        bool pick_item = true;

        if (view) {
          gfx::Rect vp = view->getViewportBounds();

          if (msg->mouse.y < vp.y) {
            int num = MAX(1, (vp.y - msg->mouse.y) / 8);
            jlistbox_select_index(widget, select-num);
            pick_item = false;
          }
          else if (msg->mouse.y >= vp.y + vp.h) {
            int num = MAX(1, (msg->mouse.y - (vp.y+vp.h-1)) / 8);
            jlistbox_select_index(widget, select+num);
            pick_item = false;
          }
        }

        if (pick_item) {
          JWidget picked;

          if (view) {
            picked = view->getViewport()->pick(msg->mouse.x, msg->mouse.y);
          }
          else {
            picked = widget->pick(msg->mouse.x, msg->mouse.y);
          }

          /* if the picked widget is a child of the list, select it */
          if (picked && widget->hasChild(picked))
            jlistbox_select_child(widget, picked);
        }

        return true;
      }
      break;

    case JM_BUTTONRELEASED:
      widget->releaseMouse();
      break;

    case JM_WHEEL: {
      View* view = View::getView(widget);
      if (view) {
        gfx::Point scroll = view->getViewScroll();
        scroll.y += (jmouse_z(1) - jmouse_z(0)) * jwidget_get_text_height(widget)*3;
        view->setViewScroll(scroll);
      }
      break;
    }

    case JM_KEYPRESSED:
      if (widget->hasFocus() && !jlist_empty(widget->children)) {
        int select = jlistbox_get_selected_index(widget);
        View* view = View::getView(widget);
        int bottom = MAX(0, jlist_length(widget->children)-1);

        switch (msg->key.scancode) {
          case KEY_UP:
            select--;
            break;
          case KEY_DOWN:
            select++;
            break;
          case KEY_HOME:
            select = 0;
            break;
          case KEY_END:
            select = bottom;
            break;
          case KEY_PGUP:
            if (view) {
              gfx::Rect vp = view->getViewportBounds();
              select -= vp.h / jwidget_get_text_height(widget);
            }
            else
              select = 0;
            break;
          case KEY_PGDN:
            if (view) {
              gfx::Rect vp = view->getViewportBounds();
              select += vp.h / jwidget_get_text_height(widget);
            }
            else
              select = bottom;
            break;
          case KEY_LEFT:
          case KEY_RIGHT:
            if (view) {
              gfx::Rect vp = view->getViewportBounds();
              gfx::Point scroll = view->getViewScroll();
              int sgn = (msg->key.scancode == KEY_LEFT) ? -1: 1;

              scroll.x += vp.w/2*sgn;

              view->setViewScroll(scroll);
            }
            break;
          default:
            return false;
        }

        jlistbox_select_index(widget, MID(0, select, bottom));
        return true;
      }
      break;

    case JM_DOUBLECLICK:
      jwidget_emit_signal(widget, JI_SIGNAL_LISTBOX_SELECT);
      return true;
  }

  return false;
}

static void listbox_request_size(JWidget widget, int *w, int *h)
{
  Size reqSize;
  JLink link;

  *w = *h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    reqSize = reinterpret_cast<Widget*>(link->data)->getPreferredSize();

    *w = MAX(*w, reqSize.w);
    *h += reqSize.h + ((link->next)? widget->child_spacing: 0);
  }

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void listbox_set_position(JWidget widget, JRect rect)
{
  Size reqSize;
  JWidget child;
  JRect cpos;
  JLink link;

  jrect_copy(widget->rc, rect);
  cpos = jwidget_get_child_rect(widget);

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    reqSize = child->getPreferredSize();

    cpos->y2 = cpos->y1+reqSize.h;
    jwidget_set_rect(child, cpos);

    cpos->y1 += jrect_h(child->rc) + widget->child_spacing;
  }

  jrect_free(cpos);
}

static void listbox_dirty_children(JWidget widget)
{
  View* view = View::getView(widget);
  JWidget child;
  JLink link;

  if (!view) {
    JI_LIST_FOR_EACH(widget->children, link)
      reinterpret_cast<JWidget>(link->data)->invalidate();
  }
  else {
    gfx::Rect vp = view->getViewportBounds();

    JI_LIST_FOR_EACH(widget->children, link) {
      child = reinterpret_cast<JWidget>(link->data);

      if (child->rc->y2 <= vp.y)
        continue;
      else if (child->rc->y1 >= vp.y+vp.h)
        break;

      child->invalidate();
    }
  }
}

static bool listitem_msg_proc(JWidget widget, Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      listitem_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS: {
      JRect crect;
      JLink link;

      jrect_copy(widget->rc, &msg->setpos.rect);
      crect = jwidget_get_child_rect(widget);

      JI_LIST_FOR_EACH(widget->children, link)
        jwidget_set_rect(reinterpret_cast<JWidget>(link->data), crect);

      jrect_free(crect);
      return true;
    }

    case JM_DRAW:
      widget->getTheme()->draw_listitem(widget, &msg->draw.rect);
      return true;
  }

  return false;
}

static void listitem_request_size(JWidget widget, int *w, int *h)
{
  Size maxSize;
  Size reqSize;
  JLink link;

  if (widget->hasText()) {
    maxSize.w = jwidget_get_text_length(widget);
    maxSize.h = jwidget_get_text_height(widget);
  }
  else
    maxSize.w = maxSize.h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    reqSize = reinterpret_cast<Widget*>(link->data)->getPreferredSize();

    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  *w = widget->border_width.l + maxSize.w + widget->border_width.r;
  *h = widget->border_width.t + maxSize.h + widget->border_width.b;
}
