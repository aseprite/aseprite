// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro/keyboard.h>
#include <math.h>

#include "gfx/point.h"
#include "gui/hook.h"
#include "gui/intern.h"
#include "gui/manager.h"
#include "gui/message.h"
#include "gui/rect.h"
#include "gui/system.h"
#include "gui/theme.h"
#include "gui/view.h"
#include "gui/widget.h"

static bool textbox_msg_proc(JWidget widget, Message* msg);
static void textbox_request_size(JWidget widget, int *w, int *h);

JWidget jtextbox_new(const char *text, int align)
{
  Widget* widget = new Widget(JI_TEXTBOX);

  jwidget_add_hook(widget, JI_TEXTBOX, textbox_msg_proc, NULL);
  jwidget_focusrest(widget, true);
  widget->setAlign(align);
  widget->setText(text);
  widget->initTheme();

  return widget;
}

static bool textbox_msg_proc(JWidget widget, Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      textbox_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_DRAW:
      widget->getTheme()->draw_textbox(widget, &msg->draw.rect);
      return true;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_SET_TEXT) {
        View* view = View::getView(widget);
        if (view)
          view->updateView();
      }
      break;

    case JM_KEYPRESSED:
      if (widget->hasFocus()) {
        View* view = View::getView(widget);
        if (view) {
          gfx::Rect vp = view->getViewportBounds();
          gfx::Point scroll = view->getViewScroll();
          int textheight = jwidget_get_text_height(widget);

          switch (msg->key.scancode) {

            case KEY_LEFT:
              scroll.x -= vp.w/2;
              view->setViewScroll(scroll);
              break;

            case KEY_RIGHT:
              scroll.x += vp.w/2;
              view->setViewScroll(scroll);
              break;

            case KEY_UP:
              scroll.y -= vp.h/2;
              view->setViewScroll(scroll);
              break;

            case KEY_DOWN:
              scroll.y += vp.h/2;
              view->setViewScroll(scroll);
              break;

            case KEY_PGUP:
              scroll.y -= (vp.h-textheight);
              view->setViewScroll(scroll);
              break;

            case KEY_PGDN:
              scroll.y += (vp.h-textheight);
              view->setViewScroll(scroll);
              break;

            case KEY_HOME:
              scroll.y = 0;
              view->setViewScroll(scroll);
              break;

            case KEY_END:
              scroll.y = jrect_h(widget->rc) - vp.h;
              view->setViewScroll(scroll);
              break;

            default:
              return false;
          }
        }
        return true;
      }
      break;

    case JM_BUTTONPRESSED: {
      View* view = View::getView(widget);
      if (view) {
        widget->captureMouse();
        jmouse_set_cursor(JI_CURSOR_SCROLL);
        return true;
      }
      break;
    }

    case JM_MOTION: {
      View* view = View::getView(widget);
      if (view && widget->hasCapture()) {
        gfx::Rect vp = view->getViewportBounds();
        gfx::Point scroll = view->getViewScroll();

        scroll.x += jmouse_x(1) - jmouse_x(0);
        scroll.y += jmouse_y(1) - jmouse_y(0);

        view->setViewScroll(scroll);

        jmouse_control_infinite_scroll(vp);
      }
      break;
    }

    case JM_BUTTONRELEASED: {
      View* view = View::getView(widget);
      if (view && widget->hasCapture()) {
        widget->releaseMouse();
        jmouse_set_cursor(JI_CURSOR_NORMAL);
        return true;
      }
      break;
    }

    case JM_WHEEL: {
      View* view = View::getView(widget);
      if (view) {
        gfx::Point scroll = view->getViewScroll();

        scroll.y += (jmouse_z(1) - jmouse_z(0)) * jwidget_get_text_height(widget)*3;

        view->setViewScroll(scroll);
      }
      break;
    }
  }

  return false;
}

static void textbox_request_size(JWidget widget, int *w, int *h)
{
  /* TODO */
/*   *w = widget->border_width.l + widget->border_width.r; */
/*   *h = widget->border_width.t + widget->border_width.b; */
  *w = 0;
  *h = 0;

  _ji_theme_textbox_draw(NULL, widget, w, h, 0, 0);

  if (widget->getAlign() & JI_WORDWRAP) {
    View* view = View::getView(widget);
    int width, min = *w;

    if (view) {
      width = view->getViewportBounds().w;
    }
    else {
      width = jrect_w(widget->rc);
    }

    *w = MAX(min, width);
    _ji_theme_textbox_draw(NULL, widget, w, h, 0, 0);

    *w = min;
  }
}
