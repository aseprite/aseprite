// Aseprite UI Library
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/textbox.h"

#include "gfx/size.h"
#include "ui/display.h"
#include "ui/intern.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"

#include <algorithm>

namespace ui {

TextBox::TextBox(const std::string& text, int align)
 : Widget(kTextBoxWidget)
{
  setBgColor(gfx::ColorNone);
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
          gfx::Rect vp = view->viewportBounds();
          gfx::Point scroll = view->viewScroll();
          int textheight = textHeight();

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
              scroll.y = bounds().h - vp.h;
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
        set_mouse_cursor(kScrollCursor);
        return true;
      }
      break;
    }

    case kMouseMoveMessage: {
      View* view = View::getView(this);
      if (view && hasCapture()) {
        gfx::Point scroll = view->viewScroll();
        gfx::Point newPos = static_cast<MouseMessage*>(msg)->position();

        scroll += m_oldPos - newPos;
        view->setViewScroll(scroll);

        m_oldPos = newPos;
      }
      break;
    }

    case kMouseUpMessage: {
      View* view = View::getView(this);
      if (view && hasCapture()) {
        releaseMouse();
        set_mouse_cursor(kArrowCursor);
        return true;
      }
      break;
    }

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (view) {
        auto mouseMsg = static_cast<MouseMessage*>(msg);
        gfx::Point scroll = view->viewScroll();

        if (mouseMsg->preciseWheel())
          scroll += mouseMsg->wheelDelta();
        else
          scroll += mouseMsg->wheelDelta() * textHeight()*3;

        view->setViewScroll(scroll);
      }
      break;
    }
  }

  return Widget::onProcessMessage(msg);
}

void TextBox::onPaint(PaintEvent& ev)
{
  theme()->paintTextBox(ev);
}

void TextBox::onSizeHint(SizeHintEvent& ev)
{
  gfx::Size borderSize = border().size();
  int w = borderSize.w;
  int h = borderSize.h;

  Theme::drawTextBox(nullptr, this, &w, &h, gfx::ColorNone, gfx::ColorNone);

  if (this->align() & WORDWRAP) {
    View* view = View::getView(this);
    int width, min = w;

    if (view)
      width = view->viewportBounds().w;
    else if (bounds().w > 0)
      width = bounds().w;
    else if (auto display = this->display())
      width = display->size().w / guiscale();
    else
      width = 0;

    w = std::max(min, width);
    Theme::drawTextBox(nullptr, this, &w, &h, gfx::ColorNone, gfx::ColorNone);
  }

  ev.setSizeHint(gfx::Size(w, h));
}

void TextBox::onSetText()
{
  View* view = View::getView(this);
  if (view)
    view->updateView();

  Widget::onSetText();
}

} // namespace ui
