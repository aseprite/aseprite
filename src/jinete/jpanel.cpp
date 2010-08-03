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

#include "jinete/jlist.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"
#include "Vaca/Size.h"

typedef struct Panel
{
  double pos;
} Panel;

static bool panel_msg_proc(JWidget widget, JMessage msg);
static void panel_request_size(JWidget widget, int *w, int *h);
static void panel_set_position(JWidget widget, JRect rect);

JWidget jpanel_new(int align)
{
  Widget* widget = new Widget(JI_PANEL);
  Panel *panel = jnew(Panel, 1);

  jwidget_add_hook(widget, JI_PANEL, panel_msg_proc, panel);
  widget->setAlign(align);

  panel->pos = 50;

  jwidget_init_theme(widget);

  return widget;
}

double jpanel_get_pos(JWidget widget)
{
  Panel* panel = reinterpret_cast<Panel*>(jwidget_get_data(widget, JI_PANEL));

  return panel->pos;
}

void jpanel_set_pos(JWidget widget, double pos)
{
  Panel* panel = reinterpret_cast<Panel*>(jwidget_get_data(widget, JI_PANEL));

  panel->pos = MID(0, pos, 100);

  jwidget_dirty(widget);
}

static bool panel_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DESTROY:
      {
	Panel* panel = reinterpret_cast<Panel*>(jwidget_get_data(widget, JI_PANEL));
	jfree(panel);
      }
      break;

    case JM_REQSIZE:
      panel_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS:
      panel_set_position(widget, &msg->setpos.rect);
      return true;

    case JM_DRAW:
      widget->theme->draw_panel(widget, &msg->draw.rect);
      return true;

    case JM_BUTTONPRESSED: 
      if (widget->isEnabled()) {
	JWidget c1, c2;
	int x1, y1, x2, y2;
	int bar, click_bar;
	JLink link;

	bar = click_bar = 0;

	JI_LIST_FOR_EACH(widget->children, link) {
	  if (link->next != widget->children->end) {
	    c1 = reinterpret_cast<JWidget>(link->data);
	    c2 = reinterpret_cast<JWidget>(link->next->data);

	    ++bar;

	    if (widget->getAlign() & JI_HORIZONTAL) {
	      x1 = c1->rc->x2;
	      y1 = widget->rc->y1;
	      x2 = c2->rc->x1;
	      y2 = widget->rc->y2;
	    }
	    else {
	      x1 = widget->rc->x1;
	      y1 = c1->rc->y2;
	      x2 = widget->rc->x2;
	      y2 = c2->rc->y1;
	    }

	    if ((msg->mouse.x >= x1) && (msg->mouse.x < x2) &&
		(msg->mouse.y >= y1) && (msg->mouse.y < y2))
	      click_bar = bar;
	  }
	}

	if (!click_bar)
	  break;

	widget->captureMouse();
	/* Continue with motion message...  */
      }
      else
	break;

    case JM_MOTION:
      if (widget->hasCapture()) {
	Panel* panel = reinterpret_cast<Panel*>(jwidget_get_data(widget, JI_PANEL));

	if (widget->getAlign() & JI_HORIZONTAL) {
	  panel->pos =
	    100.0 * (msg->mouse.x-widget->rc->x1) / jrect_w(widget->rc);
	}
	else {
	  panel->pos =
	    100.0 * (msg->mouse.y-widget->rc->y1) / jrect_h(widget->rc);
	}

	panel->pos = MID(0, panel->pos, 100);

	jwidget_set_rect(widget, widget->rc);
	jwidget_dirty(widget);
	return true;
      }
      break;

    case JM_BUTTONRELEASED:
      if (widget->hasCapture()) {
	widget->releaseMouse();
	return true;
      }
      break;

    case JM_SETCURSOR:
      if (widget->isEnabled()) {
	JWidget c1, c2;
	JLink link;
	int x1, y1, x2, y2;
	bool change_cursor = false;

	JI_LIST_FOR_EACH(widget->children, link) {
	  if (link->next != widget->children->end) {
	    c1 = reinterpret_cast<JWidget>(link->data);
	    c2 = reinterpret_cast<JWidget>(link->next->data);

	    if (widget->getAlign() & JI_HORIZONTAL) {
	      x1 = c1->rc->x2;
	      y1 = widget->rc->y1;
	      x2 = c2->rc->x1;
	      y2 = widget->rc->y2;
	    }
	    else {
	      x1 = widget->rc->x1;
	      y1 = c1->rc->y2;
	      x2 = widget->rc->x2;
	      y2 = c2->rc->y1;
	    }

	    if ((msg->mouse.x >= x1) && (msg->mouse.x < x2) &&
		(msg->mouse.y >= y1) && (msg->mouse.y < y2)) {
	      change_cursor = true;
	      break;
	    }
	  }
	}

	if (change_cursor) {
	  if (widget->getAlign() & JI_HORIZONTAL)
	    jmouse_set_cursor(JI_CURSOR_SIZE_L);
	  else
	    jmouse_set_cursor(JI_CURSOR_SIZE_T);
	  return true;
	}
	else
	  return false;
      }
      break;

  }

  return false;
}

static void panel_request_size(JWidget widget, int *w, int *h)
{
#define GET_CHILD_SIZE(w, h)			\
  do {						\
    *w = MAX(*w, reqSize.w);			\
    *h = MAX(*h, reqSize.h);			\
  } while(0)

#define FINAL_SIZE(w)					\
  do {							\
    *w *= nvis_children;				\
    *w += widget->child_spacing * (nvis_children-1);	\
  } while(0)

  int nvis_children;
  Size reqSize;
  JWidget child;
  JLink link;

  nvis_children = 0;
  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;
    if (!(child->flags & JI_HIDDEN))
      nvis_children++;
  }

  *w = *h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    if (child->flags & JI_HIDDEN)
      continue;

    reqSize = child->getPreferredSize();

    if (widget->getAlign() & JI_HORIZONTAL)
      GET_CHILD_SIZE(w, h);
    else
      GET_CHILD_SIZE(h, w);
  }

  if (nvis_children > 0) {
    if (widget->getAlign() & JI_HORIZONTAL)
      FINAL_SIZE(w);
    else
      FINAL_SIZE(h);
  }

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void panel_set_position(JWidget widget, JRect rect)
{
#define FIXUP(x, y, w, h, l, t, r, b)					\
  do {									\
    avail = jrect_##w(widget->rc) - widget->child_spacing;		\
									\
    pos->x##1 = widget->rc->x##1;					\
    pos->y##1 = widget->rc->y##1;					\
    pos->x##2 = pos->x##1 + avail*panel->pos/100;			\
    /* TODO uncomment this to make a restricted panel */		\
    /* pos->w = MID(reqSize1.w, pos->w, avail-reqSize2.w); */		\
    pos->y##2 = pos->y##1 + jrect_##h(widget->rc);			\
									\
    jwidget_set_rect(child1, pos);					\
									\
    pos->x##1 = child1->rc->x##1 + jrect_##w(child1->rc)		\
      + widget->child_spacing;						\
    pos->y##1 = widget->rc->y##1;					\
    pos->x##2 = pos->x##1 + avail - jrect_##w(child1->rc);		\
    pos->y##2 = pos->y##1 + jrect_##h(widget->rc);			\
									\
    jwidget_set_rect(child2, pos);					\
  } while(0)

  JRect pos = jrect_new(0, 0, 0, 0);
  Panel* panel = reinterpret_cast<Panel*>(jwidget_get_data(widget, JI_PANEL));
  int avail;

  jrect_copy(widget->rc, rect);

  if (jlist_length(widget->children) == 2) {
    JWidget child1 = reinterpret_cast<JWidget>(jlist_first(widget->children)->data);
    JWidget child2 = reinterpret_cast<JWidget>(jlist_first(widget->children)->next->data);
    //Size reqSize1 = child1->getPreferredSize();
    //Size reqSize2 = child2->getPreferredSize();

    if (widget->getAlign() & JI_HORIZONTAL)
      FIXUP(x, y, w, h, l, t, r, b);
    else
      FIXUP(y, x, h, w, t, l, b, r);
  }

  jrect_free(pos);
}
