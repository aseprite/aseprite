/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "jinete/jintern.h"
#include "jinete/jlist.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jregion.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"
#include "Vaca/Size.h"

#define BAR_SIZE widget->theme->scrollbar_size

typedef struct View
{
  int max_w, max_h;	/* space which the widget need in the viewport */
  int scroll_x;		/* scrolling in x and y axis */
  int scroll_y;
  unsigned hasbars : 1;
  /* --internal use-- */
  int wherepos, whereclick;
  JWidget viewport;
  JWidget scrollbar_h;
  JWidget scrollbar_v;
} View;

static void view_plain_update(JWidget widget);
static bool view_msg_proc(JWidget widget, JMessage msg);

static JWidget viewport_new();
static bool viewport_msg_proc(JWidget widget, JMessage msg);
static void viewport_needed_size(JWidget widget, int *w, int *h);
static void viewport_set_position(JWidget widget, JRect rect);

static JWidget scrollbar_new(int align);
static bool scrollbar_msg_proc(JWidget widget, JMessage msg);
static void scrollbar_info(JWidget widget, int *_pos, int *_len,
			   int *_bar_size, int *_viewport_size);

static void displace_widgets(JWidget widget, int x, int y);

JWidget jview_new()
{
  Widget* widget = new Widget(JI_VIEW);
  View* view = jnew(View, 1);

  view->viewport = viewport_new();
  view->scrollbar_h = scrollbar_new(JI_HORIZONTAL);
  view->scrollbar_v = scrollbar_new(JI_VERTICAL);
  view->hasbars = true;
  view->wherepos = 0;
  view->whereclick = 0;

  jwidget_add_hook(widget, JI_VIEW, view_msg_proc, view);
  jwidget_focusrest(widget, true);
  jwidget_add_child(widget, view->viewport);
  jview_set_size(widget, 0, 0);

  jwidget_init_theme(widget);

  view->scroll_x = -1;
  view->scroll_y = -1;

  return widget;
}

bool jview_has_bars(JWidget widget)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));

  return view->hasbars;
}

void jview_attach(JWidget widget, JWidget viewable_widget)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));

  jwidget_add_child(view->viewport, viewable_widget);
  /* TODO */
  /* jwidget_emit_signal(widget, JI_SIGNAL_VIEW_ATTACH); */
}

void jview_maxsize(JWidget widget)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));
  int req_w, req_h;

  viewport_needed_size(view->viewport, &req_w, &req_h);

  widget->min_w =
    + widget->border_width.l
    + view->viewport->border_width.l
    + req_w
    + view->viewport->border_width.r
    + widget->border_width.r;

  widget->min_h =
    + widget->border_width.t
    + view->viewport->border_width.t
    + req_h
    + view->viewport->border_width.b
    + widget->border_width.b;
}

void jview_without_bars(JWidget widget)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));

  view->hasbars = false;

  jview_update(widget);
}

void jview_set_size(JWidget widget, int w, int h)
{
#define CHECK(w, h, l, t, r, b)						\
  ((view->max_##w > jrect_##w(view->viewport->rc)			\
                    - view->viewport->border_width.l			\
		    - view->viewport->border_width.r) &&		\
   (BAR_SIZE < jrect_##w(pos)) && (BAR_SIZE < jrect_##h(pos)))

  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));
  JRect pos, rect;

  view->max_w = w;
  view->max_h = h;

  pos = jwidget_get_child_rect(widget);

  /* setup scroll-bars */
  jwidget_remove_child(widget, view->scrollbar_h);
  jwidget_remove_child(widget, view->scrollbar_v);

  if (view->hasbars) {
    if (CHECK (w, h, l, t, r, b)) {
      pos->y2 -= BAR_SIZE;
      jwidget_add_child(widget, view->scrollbar_h);

      if (CHECK (h, w, t, l, b, r)) {
	pos->x2 -= BAR_SIZE;
	if (CHECK (w, h, l, t, r, b))
	  jwidget_add_child(widget, view->scrollbar_v);
	else {
	  pos->x2 += BAR_SIZE;
	  pos->y2 += BAR_SIZE;
	  jwidget_remove_child(widget, view->scrollbar_h);
	}
      }
    }
    else if (CHECK (h, w, t, l, b, r)) {
      pos->x2 -= BAR_SIZE;
      jwidget_add_child(widget, view->scrollbar_v);

      if (CHECK (w, h, l, t, r, b)) {
        pos->y2 -= BAR_SIZE;
	if (CHECK (h, w, t, l, b, r))
	  jwidget_add_child(widget, view->scrollbar_h);
	else {
	  pos->x2 += BAR_SIZE;
	  pos->y2 += BAR_SIZE;
	  jwidget_remove_child(widget, view->scrollbar_v);
	}
      }
    }

    if (jwidget_has_child(widget, view->scrollbar_h)) {
      rect = jrect_new(pos->x1, pos->y2,
		       pos->x1+jrect_w(pos), pos->y2+BAR_SIZE);
      jwidget_set_rect(view->scrollbar_h, rect);
      jrect_free(rect);

      view->scrollbar_h->setVisible(true);
    }
    else
      view->scrollbar_h->setVisible(false);

    if (jwidget_has_child(widget, view->scrollbar_v)) {
      rect = jrect_new(pos->x2, pos->y1,
		       pos->x2+BAR_SIZE, pos->y1+jrect_h(pos));
      jwidget_set_rect(view->scrollbar_v, rect);
      jrect_free(rect);

      view->scrollbar_v->setVisible(true);
    }
    else
      view->scrollbar_v->setVisible(false);
  }

  /* setup viewport */
  jwidget_dirty(widget);
  jwidget_set_rect(view->viewport, pos);
  jview_set_scroll(widget, view->scroll_x, view->scroll_y);

  jrect_free(pos);
}

void jview_set_scroll(JWidget widget, int x, int y)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));
  int old_x = view->scroll_x;
  int old_y = view->scroll_y;
  int avail_w = jrect_w(view->viewport->rc)
    - view->viewport->border_width.l
    - view->viewport->border_width.r;
  int avail_h = jrect_h(view->viewport->rc)
    - view->viewport->border_width.t
    - view->viewport->border_width.b;

  view->scroll_x = MID(0, x, MAX(0, view->max_w - avail_w));
  view->scroll_y = MID(0, y, MAX(0, view->max_h - avail_h));

  if ((view->scroll_x == old_x) && (view->scroll_y == old_y))
    return;

  jwidget_set_rect(view->viewport, view->viewport->rc);
  jwidget_dirty(widget);
}

void jview_get_scroll(JWidget widget, int *x, int *y)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));

  *x = view->scroll_x;
  *y = view->scroll_y;
}

void jview_get_max_size(JWidget widget, int *w, int *h)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));

  *w = view->max_w;
  *h = view->max_h;
}

void jview_update(JWidget widget)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));
  JWidget vw = reinterpret_cast<JWidget>(jlist_first_data(view->viewport->children));
/*   int center_x = vw ? vw->rect->x+vw->rect->w/2: 0; */
/*   int center_y = vw ? vw->rect->y+vw->rect->h/2: 0; */
  int scroll_x = view->scroll_x;
  int scroll_y = view->scroll_y;

  view_plain_update(widget);

  if (vw)
    jview_set_scroll(widget,
		       scroll_x, scroll_y);
/* 			view->scroll_x + (vw->rect->x + vw->rect->w/2) - center_x, */
/* 			view->scroll_y + (vw->rect->y + vw->rect->h/2) - center_y); */
  else
    jview_set_scroll(widget, 0, 0);
}

JWidget jview_get_viewport(JWidget widget)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));

  return view->viewport;
}

JRect jview_get_viewport_position(JWidget widget)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));

  return jwidget_get_child_rect(view->viewport);
}

void jtheme_scrollbar_info(JWidget widget, int *pos, int *len)
{
  scrollbar_info(widget, pos, len, NULL, NULL);
}

/* for viewable widgets */
JWidget jwidget_get_view(JWidget widget)
{
  if ((widget->parent) && (widget->parent->parent) &&
      ((widget->parent->type == JI_VIEW_VIEWPORT)) &&
      ((widget->parent->parent->type == JI_VIEW)))
    return widget->parent->parent;
  else
    return 0;
}

static void view_plain_update(JWidget widget)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));
  int req_w, req_h;

  jview_set_size(widget, 0, 0);

  viewport_needed_size(view->viewport, &req_w, &req_h);
  jview_set_size(widget, req_w, req_h);

  if ((jwidget_has_child(widget, view->scrollbar_h)) ||
      (jwidget_has_child(widget, view->scrollbar_v))) {
    viewport_needed_size(view->viewport, &req_w, &req_h);
    jview_set_size(widget, req_w, req_h);
  }
}

static bool view_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DESTROY: {
      View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));

      jwidget_remove_child(widget, view->viewport);
      jwidget_remove_child(widget, view->scrollbar_h);
      jwidget_remove_child(widget, view->scrollbar_v);

      jwidget_free(view->viewport);
      jwidget_free(view->scrollbar_h);
      jwidget_free(view->scrollbar_v);

      jfree(view);
      break;
    }

    case JM_REQSIZE: {
      View* view = reinterpret_cast<View*>(jwidget_get_data(widget, JI_VIEW));

      Size viewSize = view->viewport->getPreferredSize();
      msg->reqsize.w = viewSize.w;
      msg->reqsize.h = viewSize.h;

      msg->reqsize.w += widget->border_width.l + widget->border_width.r;
      msg->reqsize.h += widget->border_width.t + widget->border_width.b;
      return true;
    }

    case JM_SETPOS:
      if (!_jwindow_is_moving()) { /* dirty trick */
	jrect_copy(widget->rc, &msg->setpos.rect);
	jview_update(widget);
      }
      else {
	displace_widgets(widget,
			 msg->setpos.rect.x1 - widget->rc->x1,
			 msg->setpos.rect.y1 - widget->rc->y1);
      }
      return true;

    case JM_DRAW:
      widget->theme->draw_view(widget, &msg->draw.rect);
      return true;

    case JM_FOCUSENTER:
    case JM_FOCUSLEAVE:
      /* TODO add something to avoid this (theme specific stuff) */
      {
	JRegion reg1 = jwidget_get_drawable_region(widget,
						   JI_GDR_CUTTOPWINDOWS);
	jregion_union(widget->update_region, widget->update_region, reg1);
	jregion_free(reg1);
      }
      break;
  }

  return false;
}

static JWidget viewport_new()
{
  Widget* widget = new Widget(JI_VIEW_VIEWPORT);

  jwidget_add_hook(widget, JI_VIEW_VIEWPORT, viewport_msg_proc, NULL);
  jwidget_init_theme(widget);

  return widget;
}

static bool viewport_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      msg->reqsize.w = widget->border_width.l + 1 + widget->border_width.r;
      msg->reqsize.h = widget->border_width.t + 1 + widget->border_width.b;
      return true;

    case JM_SETPOS:
      viewport_set_position(widget, &msg->setpos.rect);
      return true;

    case JM_DRAW:
      widget->theme->draw_view_viewport(widget, &msg->draw.rect);
      return true;
  }

  return false;
}

static void viewport_needed_size(JWidget widget, int *w, int *h)
{
  Size reqSize;
  JLink link;

  *w = *h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    reqSize = ((Widget*)link->data)->getPreferredSize();

    *w = MAX(*w, reqSize.w);
    *h = MAX(*h, reqSize.h);
  }
}

static void viewport_set_position(JWidget widget, JRect rect)
{
  int scroll_x, scroll_y;
  Size reqSize;
  JWidget child;
  JRect cpos;
  JLink link;

  jrect_copy(widget->rc, rect);

  jview_get_scroll(widget->parent, &scroll_x, &scroll_y);

  cpos = jrect_new(0, 0, 0, 0);
  cpos->x1 = widget->rc->x1 + widget->border_width.l - scroll_x;
  cpos->y1 = widget->rc->y1 + widget->border_width.t - scroll_y;

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;
    reqSize = child->getPreferredSize();

    cpos->x2 = cpos->x1 + MAX(reqSize.w, jrect_w(widget->rc)
					 - widget->border_width.l
					 - widget->border_width.r);

    cpos->y2 = cpos->y1 + MAX(reqSize.h, jrect_h(widget->rc)
					 - widget->border_width.t
					 - widget->border_width.b);

    jwidget_set_rect(child, cpos);
  }

  jrect_free(cpos);
}

static JWidget scrollbar_new(int align)
{
  Widget* widget = new Widget(JI_VIEW_SCROLLBAR);

  jwidget_add_hook(widget, JI_VIEW_SCROLLBAR, scrollbar_msg_proc, NULL);
  widget->setAlign(align);
  jwidget_init_theme(widget);

  return widget;
}

static bool scrollbar_msg_proc(JWidget widget, JMessage msg)
{
#define MOUSE_IN(x1, y1, x2, y2) \
  ((msg->mouse.x >= (x1)) && (msg->mouse.x <= (x2)) && \
   (msg->mouse.y >= (y1)) && (msg->mouse.y <= (y2)))

  switch (msg->type) {

    case JM_BUTTONPRESSED: {
      View* view = reinterpret_cast<View*>(jwidget_get_data(widget->parent, JI_VIEW));
      int x1, y1, x2, y2;
      int u1, v1, u2, v2;
      bool ret = false;
      int pos, len;

      jtheme_scrollbar_info(widget, &pos, &len);

      view->wherepos = pos;
      view->whereclick = widget->getAlign() & JI_HORIZONTAL ? msg->mouse.x:
							      msg->mouse.y;

      x1 = widget->rc->x1;
      y1 = widget->rc->y1;
      x2 = widget->rc->x2-1;
      y2 = widget->rc->y2-1;

      u1 = x1 + widget->border_width.l;
      v1 = y1 + widget->border_width.t;
      u2 = x2 - widget->border_width.r;
      v2 = y2 - widget->border_width.b;

      if (widget->getAlign() & JI_HORIZONTAL) {
	/* in the bar */
	if (MOUSE_IN(u1+pos, v1, u1+pos+len-1, v2)) {
	  /* capture mouse */
	}
	/* left */
	else if (MOUSE_IN(x1, y1, u1+pos-1, y2)) {
	  jview_set_scroll(widget->parent,
			   view->scroll_x - jrect_w(view->viewport->rc)/2,
			   view->scroll_y);
	  ret = true;
	}
	/* right */
	else if (MOUSE_IN(u1+pos+len, y1, x2, y2)) {
	  jview_set_scroll(widget->parent,
			   view->scroll_x + jrect_w(view->viewport->rc)/2,
			   view->scroll_y);
	  ret = true;
	}
      }
      else {
	/* in the bar */
	if (MOUSE_IN(u1, v1+pos, u2, v1+pos+len-1)) {
	  /* capture mouse */
	}
	/* left */
	else if (MOUSE_IN(x1, y1, x2, v1+pos-1)) {
	  jview_set_scroll(widget->parent,
			   view->scroll_x,
			   view->scroll_y - jrect_h(view->viewport->rc)/2);
	  ret = true;
	}
	/* right */
	else if (MOUSE_IN(x1, v1+pos+len, x2, y2)) {
	  jview_set_scroll(widget->parent,
			   view->scroll_x,
			   view->scroll_y + jrect_h(view->viewport->rc)/2);
	  ret = true;
	}
      }

      if (ret)
	return ret;

      widget->setSelected(true);
      widget->captureMouse();

      // continue to JM_MOTION handler...
    }

    case JM_MOTION:
      if (widget->hasCapture()) {
	View* view = reinterpret_cast<View*>(jwidget_get_data(widget->parent, JI_VIEW));
	int pos, len, bar_size, viewport_size;
	int old_pos;

	scrollbar_info(widget, &pos, &len,
		       &bar_size, &viewport_size);
	old_pos = pos;

	if (bar_size > len) {
	  if (widget->getAlign() & JI_HORIZONTAL) {
	    pos = (view->wherepos + msg->mouse.x - view->whereclick);
	    pos = MID(0, pos, bar_size - len);

	    jview_set_scroll
	      (widget->parent,
	       (view->max_w - viewport_size) * pos / (bar_size - len),
	       view->scroll_y);
	  }
	  else {
	    pos = (view->wherepos + msg->mouse.y - view->whereclick);
	    pos = MID(0, pos, bar_size - len);

	    jview_set_scroll
	      (widget->parent,
	       view->scroll_x,
	       (view->max_h - viewport_size) * pos / (bar_size - len));
	  }
	}
      }
      break;

    case JM_BUTTONRELEASED:
      widget->setSelected(false);
      widget->releaseMouse();
      break;

    case JM_MOUSEENTER:
    case JM_MOUSELEAVE:
      /* TODO add something to avoid this (theme specific stuff) */
      jwidget_invalidate(widget);
      break;

    case JM_DRAW:
      widget->theme->draw_view_scrollbar(widget, &msg->draw.rect);
      return true;
  }

  return false;
}

static void scrollbar_info(JWidget widget, int *_pos, int *_len,
			   int *_bar_size, int *_viewport_size)
{
  View* view = reinterpret_cast<View*>(jwidget_get_data(widget->parent, JI_VIEW));
  int bar_size, viewport_size;
  int pos, len, max, scroll;
  int border_width;

  if (widget->getAlign() & JI_HORIZONTAL) {
    max = view->max_w;
    scroll = view->scroll_x;
    bar_size = jrect_w(widget->rc)
      - widget->border_width.l
      - widget->border_width.r;
    viewport_size = jrect_w(view->viewport->rc)
      - view->viewport->border_width.l
      - view->viewport->border_width.r;
    border_width = widget->border_width.t + widget->border_width.b;
  }
  else {
    max = view->max_h;
    scroll = view->scroll_y;
    bar_size = jrect_h(widget->rc)
      - widget->border_width.t
      - widget->border_width.b;
    viewport_size = jrect_h(view->viewport->rc)
      - view->viewport->border_width.t
      - view->viewport->border_width.b;
    border_width = widget->border_width.l + widget->border_width.r;
  }

  if (max <= viewport_size) {
    len = bar_size;
    pos = 0;
  }
  else {
    len = bar_size - (max-viewport_size);
    len = MID(BAR_SIZE*2-border_width, len, bar_size);
    pos = (bar_size-len) * scroll / (max-viewport_size);
    pos = MID(0, pos, bar_size-len);
  }

  if (_pos) *_pos = pos;
  if (_len) *_len = len;
  if (_bar_size) *_bar_size = bar_size;
  if (_viewport_size) *_viewport_size = viewport_size;
}

static void displace_widgets(JWidget widget, int x, int y)
{
  JLink link;

  jrect_displace(widget->rc, x, y);

  JI_LIST_FOR_EACH(widget->children, link)
    displace_widgets(reinterpret_cast<JWidget>(link->data), x, y);
}
