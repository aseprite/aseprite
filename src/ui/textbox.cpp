// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

TextBox::TextBox(const base::string& text, int align)
 : Widget(kTextBoxWidget)
{
  setFocusStop(true);
  setAlign(align);
  setText(text);
  initTheme();
}

bool TextBox::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kKeyDownMessage:
      if (hasFocus()) {
        View* view = View::getView(this);
        if (view) {
          gfx::Rect vp = view->getViewportBounds();
          gfx::Point scroll = view->getViewScroll();
          int textheight = getTextHeight();

          switch (static_cast<KeyMessage*>(msg)->scancode()) {

            case kKeyLeft:
              scroll.x -= vp.w/2;
              view->setViewScroll(scroll);
              break;

            case kKeyRight:
              scroll.x += vp.w/2;
              view->setViewScroll(scroll);
              break;

            case kKeyUp:
              scroll.y -= vp.h/2;
              view->setViewScroll(scroll);
              break;

            case kKeyDown:
              scroll.y += vp.h/2;
              view->setViewScroll(scroll);
              break;

            case kKeyPageUp:
              scroll.y -= (vp.h-textheight);
              view->setViewScroll(scroll);
              break;

            case kKeyPageDown:
              scroll.y += (vp.h-textheight);
              view->setViewScroll(scroll);
              break;

            case kKeyHome:
              scroll.y = 0;
              view->setViewScroll(scroll);
              break;

            case kKeyEnd:
              scroll.y = getBounds().h - vp.h;
              view->setViewScroll(scroll);
              break;

            default:
              return Widget::onProcessMessage(msg);
          }
        }
        return true;
      }
      break;

    case kMouseDownMessage: {
      View* view = View::getView(this);
      if (view) {
        captureMouse();
        m_oldPos = static_cast<MouseMessage*>(msg)->position();
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
        gfx::Point newPos = static_cast<MouseMessage*>(msg)->position();

        scroll += m_oldPos - newPos;
        view->setViewScroll(scroll);

        m_oldPos = ui::control_infinite_scroll(this, vp, newPos);
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

        scroll.y += (jmouse_z(1) - jmouse_z(0)) * getTextHeight()*3;

        view->setViewScroll(scroll);
      }
      break;
    }
  }

  return Widget::onProcessMessage(msg);
}

void TextBox::onPaint(PaintEvent& ev)
{
  getTheme()->paintTextBox(ev);
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
      width = getBounds().w;
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
