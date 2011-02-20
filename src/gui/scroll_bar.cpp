// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gfx/size.h"
#include "gui/message.h"
#include "gui/scroll_bar.h"
#include "gui/theme.h"
#include "gui/view.h"

using namespace gfx;

// Internal stuff shared by all scroll-bars (as the user cannot move
// two scroll-bars at the same time).
int ScrollBar::m_wherepos = 0;
int ScrollBar::m_whereclick = 0;

ScrollBar::ScrollBar(int align)
  : Widget(JI_VIEW_SCROLLBAR)
  , m_pos(0)
  , m_size(0)
{
  setAlign(align);
  initTheme();
}

void ScrollBar::setPos(int pos)
{
  m_pos = pos;
}

void ScrollBar::setSize(int size)
{
  m_size = size;
}

void ScrollBar::getScrollBarThemeInfo(int* pos, int* len)
{
  getScrollBarInfo(pos, len, NULL, NULL);
}

bool ScrollBar::onProcessMessage(JMessage msg)
{
#define MOUSE_IN(x1, y1, x2, y2) \
  ((msg->mouse.x >= (x1)) && (msg->mouse.x <= (x2)) && \
   (msg->mouse.y >= (y1)) && (msg->mouse.y <= (y2)))

  switch (msg->type) {

    case JM_BUTTONPRESSED: {
      View* view = static_cast<View*>(getParent());
      int x1, y1, x2, y2;
      int u1, v1, u2, v2;
      bool ret = false;
      int pos, len;

      getScrollBarThemeInfo(&pos, &len);

      m_wherepos = pos;
      m_whereclick = getAlign() & JI_HORIZONTAL ?
	msg->mouse.x:
	msg->mouse.y;

      x1 = this->rc->x1;
      y1 = this->rc->y1;
      x2 = this->rc->x2-1;
      y2 = this->rc->y2-1;

      u1 = x1 + this->border_width.l;
      v1 = y1 + this->border_width.t;
      u2 = x2 - this->border_width.r;
      v2 = y2 - this->border_width.b;

      Point scroll = view->getViewScroll();

      if (this->getAlign() & JI_HORIZONTAL) {
	// in the bar
	if (MOUSE_IN(u1+pos, v1, u1+pos+len-1, v2)) {
	  // capture mouse
	}
	// left
	else if (MOUSE_IN(x1, y1, u1+pos-1, y2)) {
	  scroll.x -= jrect_w(view->getViewport()->rc)/2;
	  ret = true;
	}
	// right
	else if (MOUSE_IN(u1+pos+len, y1, x2, y2)) {
	  scroll.x += jrect_w(view->getViewport()->rc)/2;
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
	  scroll.y -= jrect_h(view->getViewport()->rc)/2;
	  ret = true;
	}
	// right
	else if (MOUSE_IN(x1, v1+pos+len, x2, y2)) {
	  scroll.y += jrect_h(view->getViewport()->rc)/2;
	  ret = true;
	}
      }

      if (ret) {
	view->setViewScroll(scroll);
	return ret;
      }

      setSelected(true);
      captureMouse();

      // continue to JM_MOTION handler...
    }

    case JM_MOTION:
      if (hasCapture()) {
	View* view = static_cast<View*>(getParent());
	int pos, len, bar_size, viewport_size;
	int old_pos;

	getScrollBarInfo(&pos, &len, &bar_size, &viewport_size);
	old_pos = pos;

	if (bar_size > len) {
	  Point scroll = view->getViewScroll();

	  if (this->getAlign() & JI_HORIZONTAL) {
	    pos = (m_wherepos + msg->mouse.x - m_whereclick);
	    pos = MID(0, pos, bar_size - len);

	    scroll.x = (m_size - viewport_size) * pos / (bar_size - len);
	    view->setViewScroll(scroll);
	  }
	  else {
	    pos = (m_wherepos + msg->mouse.y - m_whereclick);
	    pos = MID(0, pos, bar_size - len);

	    scroll.y = (m_size - viewport_size) * pos / (bar_size - len);
	    view->setViewScroll(scroll);
	  }
	}
      }
      break;

    case JM_BUTTONRELEASED:
      setSelected(false);
      releaseMouse();
      break;

    case JM_MOUSEENTER:
    case JM_MOUSELEAVE:
      // TODO add something to avoid this (theme specific stuff)
      invalidate();
      break;

    case JM_DRAW:
      getTheme()->draw_view_scrollbar(this, &msg->draw.rect);
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void ScrollBar::getScrollBarInfo(int *_pos, int *_len, int *_bar_size, int *_viewport_size)
{
  View* view = static_cast<View*>(getParent());
  int bar_size, viewport_size;
  int pos, len;
  int border_width;

  if (this->getAlign() & JI_HORIZONTAL) {
    bar_size = jrect_w(this->rc);
    viewport_size = view->getVisibleSize().w;
    border_width = this->border_width.t + this->border_width.b;
  }
  else {
    bar_size = jrect_h(this->rc);
    viewport_size = view->getVisibleSize().h;
    border_width = this->border_width.l + this->border_width.r;
  }

  if (m_size <= viewport_size) {
    len = bar_size;
    pos = 0;
  }
  else {
    len = bar_size - (m_size-viewport_size);
    len = MID(getTheme()->scrollbar_size*2-border_width, len, bar_size);
    pos = (bar_size-len) * m_pos / (m_size-viewport_size);
    pos = MID(0, pos, bar_size-len);
  }

  if (_pos) *_pos = pos;
  if (_len) *_len = len;
  if (_bar_size) *_bar_size = bar_size;
  if (_viewport_size) *_viewport_size = viewport_size;
}
