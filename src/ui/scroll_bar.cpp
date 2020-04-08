// Aseprite UI Library
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/clamp.h"
#include "gfx/size.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/scroll_bar.h"
#include "ui/theme.h"

namespace ui {

using namespace gfx;

// Internal stuff shared by all scroll-bars (as the user cannot move
// two scroll-bars at the same time).
int ScrollBar::m_wherepos = 0;
int ScrollBar::m_whereclick = 0;

ScrollBar::ScrollBar(int align, ScrollableViewDelegate* delegate)
  : Widget(kViewScrollbarWidget)
  , m_delegate(delegate)
  , m_thumbStyle(nullptr)
  , m_barWidth(0)
  , m_pos(0)
  , m_size(0)
{
  setAlign(align);
  initTheme();
}

void ScrollBar::setPos(int pos)
{
  if (m_pos != pos) {
    m_pos = pos;
    invalidate();
  }
}

void ScrollBar::setSize(int size)
{
  if (m_size != size) {
    m_size = size;
    invalidate();
  }
}

void ScrollBar::getScrollBarThemeInfo(int* pos, int* len)
{
  getScrollBarInfo(pos, len, nullptr, nullptr);
}

bool ScrollBar::onProcessMessage(Message* msg)
{
#define MOUSE_IN(x1, y1, x2, y2) \
  ((mousePos.x >= (x1)) && (mousePos.x <= (x2)) && \
   (mousePos.y >= (y1)) && (mousePos.y <= (y2)))

  switch (msg->type()) {

    case kMouseDownMessage: {
      gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
      int x1, y1, x2, y2;
      int u1, v1, u2, v2;
      bool ret = false;
      int pos, len;

      getScrollBarThemeInfo(&pos, &len);

      m_wherepos = pos;
      m_whereclick = (align() & HORIZONTAL) ?
        mousePos.x:
        mousePos.y;

      x1 = bounds().x;
      y1 = bounds().y;
      x2 = bounds().x2()-1;
      y2 = bounds().y2()-1;

      u1 = x1 + border().left();
      v1 = y1 + border().top();
      u2 = x2 - border().right();
      v2 = y2 - border().bottom();

      Point scroll = m_delegate->viewScroll();

      if (align() & HORIZONTAL) {
        // in the bar
        if (MOUSE_IN(u1+pos, v1, u1+pos+len-1, v2)) {
          // capture mouse
        }
        // left
        else if (MOUSE_IN(x1, y1, u1+pos-1, y2)) {
          scroll.x -= m_delegate->visibleSize().w/2;
          ret = true;
        }
        // right
        else if (MOUSE_IN(u1+pos+len, y1, x2, y2)) {
          scroll.x += m_delegate->visibleSize().w/2;
          ret = true;
        }
      }
      else {
        // in the bar
        if (MOUSE_IN(u1, v1+pos, u2, v1+pos+len-1)) {
          // capture mouse
        }
        // left
        else if (MOUSE_IN(x1, y1, x2, v1+pos-1)) {
          scroll.y -= m_delegate->visibleSize().h/2;
          ret = true;
        }
        // right
        else if (MOUSE_IN(x1, v1+pos+len, x2, y2)) {
          scroll.y += m_delegate->visibleSize().h/2;
          ret = true;
        }
      }

      if (ret) {
        m_delegate->setViewScroll(scroll);
        return ret;
      }

      setSelected(true);
      captureMouse();

      // continue to kMouseMoveMessage handler...
    }

    case kMouseMoveMessage:
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        int pos, len, bar_size, viewport_size;

        getScrollBarInfo(&pos, &len, &bar_size, &viewport_size);

        if (bar_size > len) {
          Point scroll = m_delegate->viewScroll();

          if (align() & HORIZONTAL) {
            pos = (m_wherepos + mousePos.x - m_whereclick);
            pos = base::clamp(pos, 0, bar_size - len);

            scroll.x = (m_size - viewport_size) * pos / (bar_size - len);
          }
          else {
            pos = (m_wherepos + mousePos.y - m_whereclick);
            pos = base::clamp(pos, 0, bar_size - len);

            scroll.y = (m_size - viewport_size) * pos / (bar_size - len);
          }

          m_delegate->setViewScroll(scroll);
        }
        return true;
      }
      break;

    case kMouseUpMessage:
      setSelected(false);
      releaseMouse();
      break;

    case kMouseEnterMessage:
    case kMouseLeaveMessage:
      // TODO add something to avoid this (theme specific stuff)
      invalidate();
      break;
  }

  return Widget::onProcessMessage(msg);
}

void ScrollBar::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);
  m_barWidth = theme()->getScrollbarSize();
}

void ScrollBar::onPaint(PaintEvent& ev)
{
  gfx::Rect thumbBounds = clientBounds();
  if (align() & HORIZONTAL)
    getScrollBarThemeInfo(&thumbBounds.x, &thumbBounds.w);
  else
    getScrollBarThemeInfo(&thumbBounds.y, &thumbBounds.h);

  theme()->paintScrollBar(
    ev.graphics(), this, style(), thumbStyle(),
    clientBounds(), thumbBounds);
}

void ScrollBar::getScrollBarInfo(int *_pos, int *_len, int *_bar_size, int *_viewport_size)
{
  int bar_size, viewport_size;
  int pos, len;
  int border_width;

  if (align() & HORIZONTAL) {
    bar_size = bounds().w;
    viewport_size = m_delegate->visibleSize().w;
    border_width = border().height();
  }
  else {
    bar_size = bounds().h;
    viewport_size = m_delegate->visibleSize().h;
    border_width = border().width();
  }

  if (m_size <= viewport_size) {
    len = bar_size;
    pos = 0;
  }
  else if (m_size > 0) {
    len = bar_size * viewport_size / m_size;
    len = base::clamp(len, theme()->getScrollbarSize()*2-border_width, bar_size);
    pos = (bar_size-len) * m_pos / (m_size-viewport_size);
    pos = base::clamp(pos, 0, bar_size-len);
  }
  else {
    len = pos = 0;
  }

  if (_pos) *_pos = pos;
  if (_len) *_len = len;
  if (_bar_size) *_bar_size = bar_size;
  if (_viewport_size) *_viewport_size = viewport_size;
}

} // namespace ui
