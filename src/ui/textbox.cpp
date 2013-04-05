// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/textbox.h"

#include "gfx/size.h"
#include "ui/intern.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"

#include <allegro/keyboard.h>

namespace ui {

TextBox::TextBox(const char* text, int align)
 : Widget(kTextBoxWidget)
{
  setFocusStop(true);
  setAlign(align);
  setText(text);
  initTheme();
}

bool TextBox::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case kPaintMessage:
      getTheme()->draw_textbox(this, &msg->draw.rect);
      return true;

    case kKeyDownMessage:
      if (hasFocus()) {
        View* view = View::getView(this);
        if (view) {
          gfx::Rect vp = view->getViewportBounds();
          gfx::Point scroll = view->getViewScroll();
          int textheight = jwidget_get_text_height(this);

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
              scroll.y = jrect_h(this->rc) - vp.h;
              view->setViewScroll(scroll);
              break;

            default:
              return false;
          }
        }
        return true;
      }
      break;

    case kMouseDownMessage: {
      View* view = View::getView(this);
      if (view) {
        captureMouse();
        jmouse_set_cursor(kScrollCursor);
        return true;
      }
      break;
    }

    case kMouseMoveMessage: {
      View* view = View::getView(this);
      if (view && hasCapture()) {
        gfx::Rect vp = view->getViewportBounds();
        gfx::Point scroll = view->getViewScroll();

        scroll.x += jmouse_x(1) - jmouse_x(0);
        scroll.y += jmouse_y(1) - jmouse_y(0);

        view->setViewScroll(scroll);

        jmouse_control_infinite_scroll(vp);
      }
      break;
    }

    case kMouseUpMessage: {
      View* view = View::getView(this);
      if (view && hasCapture()) {
        releaseMouse();
        jmouse_set_cursor(kArrowCursor);
        return true;
      }
      break;
    }

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (view) {
        gfx::Point scroll = view->getViewScroll();

        scroll.y += (jmouse_z(1) - jmouse_z(0)) * jwidget_get_text_height(this)*3;

        view->setViewScroll(scroll);
      }
      break;
    }
  }

  return Widget::onProcessMessage(msg);
}

void TextBox::onPreferredSize(PreferredSizeEvent& ev)
{
  int w = 0;
  int h = 0;

  // TODO is it necessary?
  //w = widget->border_width.l + widget->border_width.r;
  //h = widget->border_width.t + widget->border_width.b;

  drawTextBox(NULL, this, &w, &h, ColorNone, ColorNone);

  if (this->getAlign() & JI_WORDWRAP) {
    View* view = View::getView(this);
    int width, min = w;

    if (view) {
      width = view->getViewportBounds().w;
    }
    else {
      width = jrect_w(this->rc);
    }

    w = MAX(min, width);
    drawTextBox(NULL, this, &w, &h, ColorNone, ColorNone);

    w = min;
  }

  ev.setPreferredSize(gfx::Size(w, h));
}

void TextBox::onSetText()
{
  View* view = View::getView(this);
  if (view)
    view->updateView();

  Widget::onSetText();
}

} // namespace ui
